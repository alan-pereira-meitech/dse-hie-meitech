#pragma once

#include "dse/jet_command.hpp"

#include <chrono>
#include <cstdint>
#include <memory>
#include <optional>
#include <string>
#include <vector>

namespace dse {

class JetConnection {
  public:
    using milliseconds = std::chrono::milliseconds;

    JetConnection(std::string host, std::uint16_t port = 80,
                  milliseconds timeout = milliseconds{5000});

    JetConnection(const JetConnection&) = delete;
    JetConnection& operator=(const JetConnection&) = delete;
    JetConnection(JetConnection&&) noexcept;
    JetConnection& operator=(JetConnection&&) noexcept;
    ~JetConnection();

    void connect();
    void disconnect();
    bool is_connected() const noexcept;

    std::string read(const JetCommand& command);
    int read_integer(const JetCommand& command);

    void write(const JetCommand& command, const std::string& value);
    void write_integer(const JetCommand& command, int value);

  private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
};

}  // namespace dse
