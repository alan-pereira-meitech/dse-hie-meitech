#include "dse/jet_connection.hpp"

#include <cerrno>
#include <cstring>
#include <sstream>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

#ifdef _WIN32
#include <WS2tcpip.h>
#include <WinSock2.h>
#pragma comment(lib, "Ws2_32.lib")
#else
#include <arpa/inet.h>
#include <fcntl.h>
#include <netdb.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>
#endif

namespace dse {
namespace {

#ifdef _WIN32
using socket_t = SOCKET;
constexpr socket_t kInvalidSocket = INVALID_SOCKET;
#else
using socket_t = int;
constexpr socket_t kInvalidSocket = -1;
#endif

class SocketError : public std::runtime_error {
  public:
    explicit SocketError(const std::string& message)
        : std::runtime_error(message) {}
};

void close_socket(socket_t sock) {
#ifdef _WIN32
    if (sock != kInvalidSocket) {
        ::closesocket(sock);
    }
#else
    if (sock != kInvalidSocket) {
        ::close(sock);
    }
#endif
}

bool set_blocking(socket_t sock, bool should_block) {
#ifdef _WIN32
    u_long mode = should_block ? 0 : 1;
    return ::ioctlsocket(sock, FIONBIO, &mode) == 0;
#else
    int flags = ::fcntl(sock, F_GETFL, 0);
    if (flags < 0) {
        return false;
    }
    if (should_block) {
        flags &= ~O_NONBLOCK;
    } else {
        flags |= O_NONBLOCK;
    }
    return ::fcntl(sock, F_SETFL, flags) == 0;
#endif
}

void set_socket_timeout(socket_t sock, std::chrono::milliseconds timeout) {
#ifdef _WIN32
    DWORD to = static_cast<DWORD>(timeout.count());
    ::setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, reinterpret_cast<const char*>(&to), sizeof(to));
    ::setsockopt(sock, SOL_SOCKET, SO_SNDTIMEO, reinterpret_cast<const char*>(&to), sizeof(to));
#else
    timeval tv{};
    tv.tv_sec = static_cast<long>(timeout.count() / 1000);
    tv.tv_usec = static_cast<long>((timeout.count() % 1000) * 1000);
    ::setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));
    ::setsockopt(sock, SOL_SOCKET, SO_SNDTIMEO, &tv, sizeof(tv));
#endif
}

std::string errno_string(int err) {
#ifdef _WIN32
    char* message = nullptr;
    FormatMessageA(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
                   nullptr, err, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), reinterpret_cast<LPSTR>(&message), 0, nullptr);
    std::string result = message ? message : "";
    if (message) {
        LocalFree(message);
    }
    return result;
#else
    return std::strerror(err);
#endif
}

std::string build_request(const std::string& method, const std::string& host, const std::string& path,
                          const std::string& body) {
    std::ostringstream request;
    request << method << " /jet/vars/" << path << " HTTP/1.1\r\n";
    request << "Host: " << host << "\r\n";
    request << "Accept: */*\r\n";
    request << "Connection: keep-alive\r\n";
    if (!body.empty()) {
        request << "Content-Type: application/json\r\n";
        request << "Content-Length: " << body.size() << "\r\n";
    }
    request << "\r\n";
    request << body;
    return request.str();
}



}  // namespace

struct JetConnection::Impl {
    std::string host;
    std::uint16_t port;
    std::chrono::milliseconds timeout;
    socket_t socket{ kInvalidSocket };
#ifdef _WIN32
    bool wsa_initialised{false};
#endif

    Impl(std::string host_address, std::uint16_t port_number, std::chrono::milliseconds timeout_value)
        : host(std::move(host_address)), port(port_number), timeout(timeout_value) {}

    ~Impl() { disconnect(); }

    void connect();
    void disconnect();
    bool is_connected() const noexcept { return socket != kInvalidSocket; }

    std::string execute(const std::string& method, const JetCommand& command, const std::string& body);

    void send_all(const std::string& payload);
    std::string receive_response();
};

void JetConnection::Impl::connect() {
    if (is_connected()) {
        return;
    }

#ifdef _WIN32
    if (!wsa_initialised) {
        WSADATA wsa_data;
        const int result = ::WSAStartup(MAKEWORD(2, 2), &wsa_data);
        if (result != 0) {
            throw SocketError("WSAStartup failed: " + std::to_string(result));
        }
        wsa_initialised = true;
    }
#endif

    struct addrinfo hints {};
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = IPPROTO_TCP;

    struct addrinfo* addr_list = nullptr;
    const std::string port_string = std::to_string(port);
    const int addr_result = ::getaddrinfo(host.c_str(), port_string.c_str(), &hints, &addr_list);
    if (addr_result != 0) {
#ifdef _WIN32
        throw SocketError("getaddrinfo failed: " + std::to_string(addr_result));
#else
        throw SocketError("getaddrinfo failed: " + std::string(gai_strerror(addr_result)));
#endif
    }

    socket_t connected_socket = kInvalidSocket;
    for (addrinfo* addr = addr_list; addr != nullptr; addr = addr->ai_next) {
        socket_t candidate = ::socket(addr->ai_family, addr->ai_socktype, addr->ai_protocol);
        if (candidate == kInvalidSocket) {
            continue;
        }

        if (!set_blocking(candidate, false)) {
            close_socket(candidate);
            continue;
        }

        const int connect_result = ::connect(candidate, addr->ai_addr, static_cast<int>(addr->ai_addrlen));
        if (connect_result == 0) {
            set_blocking(candidate, true);
            connected_socket = candidate;
            break;
        }

        int last_error = 0;
#ifdef _WIN32
        last_error = ::WSAGetLastError();
        if (last_error != WSAEWOULDBLOCK && last_error != WSAEINPROGRESS) {
            close_socket(candidate);
            continue;
        }
#else
        last_error = errno;
        if (last_error != EINPROGRESS) {
            close_socket(candidate);
            continue;
        }
#endif

        fd_set write_set;
        FD_ZERO(&write_set);
        FD_SET(candidate, &write_set);

        timeval tv{};
        tv.tv_sec = static_cast<long>(timeout.count() / 1000);
        tv.tv_usec = static_cast<long>((timeout.count() % 1000) * 1000);

        const int select_result = ::select(static_cast<int>(candidate + 1), nullptr, &write_set, nullptr, &tv);
        if (select_result <= 0) {
            close_socket(candidate);
            continue;
        }

        int sock_error = 0;
        socklen_t len = sizeof(sock_error);
        if (::getsockopt(candidate, SOL_SOCKET, SO_ERROR, reinterpret_cast<char*>(&sock_error), &len) != 0 || sock_error != 0) {
            close_socket(candidate);
            continue;
        }

        set_blocking(candidate, true);
        connected_socket = candidate;
        break;
    }

    ::freeaddrinfo(addr_list);

    if (connected_socket == kInvalidSocket) {
        throw SocketError("unable to connect to " + host + ":" + std::to_string(port));
    }

    socket = connected_socket;
    set_socket_timeout(socket, timeout);
}

void JetConnection::Impl::disconnect() {
    if (!is_connected()) {
        return;
    }
    close_socket(socket);
    socket = kInvalidSocket;
}

std::string JetConnection::Impl::execute(const std::string& method, const JetCommand& command, const std::string& body) {
    if (!is_connected()) {
        connect();
    }

    const std::string request = build_request(method, host, command.path, body);
    send_all(request);
    return receive_response();
}

void JetConnection::Impl::send_all(const std::string& payload) {
    const char* data = payload.data();
    std::size_t remaining = payload.size();
    while (remaining > 0) {
        const int sent = ::send(socket, data, static_cast<int>(remaining), 0);
        if (sent <= 0) {
#ifdef _WIN32
            const int error_code = ::WSAGetLastError();
#else
            const int error_code = errno;
#endif
            throw SocketError("send failed: " + errno_string(error_code));
        }
        data += sent;
        remaining -= static_cast<std::size_t>(sent);
    }
}

std::string JetConnection::Impl::receive_response() {
    std::string response;
    std::vector<char> buffer(4096);

    while (true) {
        const int received = ::recv(socket, buffer.data(), static_cast<int>(buffer.size()), 0);
        if (received == 0) {
            break;
        }
        if (received < 0) {
#ifdef _WIN32
            const int error_code = ::WSAGetLastError();
#else
            const int error_code = errno;
#endif
            throw SocketError("recv failed: " + errno_string(error_code));
        }
        response.append(buffer.data(), received);

        if (response.find("\r\n\r\n") != std::string::npos) {
            break;
        }
    }

    const auto header_end = response.find("\r\n\r\n");
    if (header_end == std::string::npos) {
        throw SocketError("invalid HTTP response");
    }

    std::size_t content_length = 0;
    std::istringstream header_stream(response.substr(0, header_end));
    std::string line;
    std::getline(header_stream, line);  // status line
    std::istringstream status_line(line);
    std::string http_version;
    unsigned int status_code = 0;
    status_line >> http_version >> status_code;
    if (status_code >= 400) {
        throw SocketError("HTTP error: " + std::to_string(status_code));
    }

    while (std::getline(header_stream, line)) {
        if (line.rfind("Content-Length:", 0) == 0) {
            const auto pos = line.find(':');
            if (pos != std::string::npos) {
                content_length = static_cast<std::size_t>(std::stoul(line.substr(pos + 1)));
            }
        }
    }

    std::string body = response.substr(header_end + 4);
    while (content_length != 0 && body.size() < content_length) {
        const int received = ::recv(socket, buffer.data(), static_cast<int>(buffer.size()), 0);
        if (received <= 0) {
#ifdef _WIN32
            const int error_code = ::WSAGetLastError();
#else
            const int error_code = errno;
#endif
            throw SocketError("recv failed: " + errno_string(error_code));
        }
        body.append(buffer.data(), received);
    }

    if (content_length != 0 && body.size() > content_length) {
        body.resize(content_length);
    }

    return body;
}

JetConnection::JetConnection(std::string host, std::uint16_t port, milliseconds timeout)
    : impl_(std::make_unique<Impl>(std::move(host), port, timeout)) {}

JetConnection::JetConnection(JetConnection&& other) noexcept = default;
JetConnection& JetConnection::operator=(JetConnection&& other) noexcept = default;

JetConnection::~JetConnection() = default;

void JetConnection::connect() { impl_->connect(); }

void JetConnection::disconnect() { impl_->disconnect(); }

bool JetConnection::is_connected() const noexcept { return impl_->is_connected(); }

std::string JetConnection::read(const JetCommand& command) {
    return impl_->execute("GET", command, "");
}

int JetConnection::read_integer(const JetCommand& command) {
    const std::string response = read(command);
    return std::stoi(response);
}

void JetConnection::write(const JetCommand& command, const std::string& value) {
    impl_->execute("PUT", command, value);
}

void JetConnection::write_integer(const JetCommand& command, int value) {
    impl_->execute("PUT", command, std::to_string(value));
}

}  // namespace dse
