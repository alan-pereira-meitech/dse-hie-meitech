#include "jetbus/client.hpp"
#include "jetbus/commands.hpp"

#include <algorithm>
#include <atomic>
#include <chrono>
#include <csignal>
#include <cstdlib>
#include <iomanip>
#include <iostream>
#include <limits>
#include <mutex>
#include <optional>
#include <string>
#include <thread>
#include <vector>

using SteadyClock = std::chrono::steady_clock;

static std::atomic<bool> g_run{true};
static void on_sigint(int) { g_run.store(false); }

int main(int argc, char** argv) {
    if (argc < 2) {
        std::cerr << "Uso: " << argv[0] << " <ip-ou-url> [duracao-segundos]\n";
        std::cerr << "Exemplos:\n  " << argv[0] << " 192.168.1.100 15\n  "
                  << argv[0] << " ws://192.168.1.100/jet/canopen 10" << std::endl;
        return 1;
    }

    std::signal(SIGINT, on_sigint);

    const std::string arg = argv[1];
    int duration_sec = (argc >= 3) ? std::max(1, std::atoi(argv[2])) : 10;

    jetbus::JetBusClient::Options opts;
    opts.enable_debug_logs = (std::getenv("JETBUS_DEBUG") != nullptr);
    opts.url = (arg.rfind("ws://", 0) == 0 || arg.rfind("wss://", 0) == 0)
                 ? arg
                 : ("ws://" + arg + "/jet/canopen");

    // Path de GROSS: permite override por env var; caso contrário usa o mapeado
    std::string gross_path = jetbus::commands::cia461_gross_value().path;
    if (const char* p = std::getenv("JETBUS_GROSS_PATH")) {
        gross_path = p;
    }

    std::vector<double> intervals_ms;
    std::optional<SteadyClock::time_point> last_tp;
    std::mutex mtx;

    jetbus::JetBusClient client(opts);
    client.set_data_callback([&](const std::string& path, const std::string&, jetbus::JetEventType) {
        if (path != gross_path) return;
    const auto now = SteadyClock::now();
        std::lock_guard<std::mutex> lk(mtx);
        if (last_tp) {
            const auto dt = std::chrono::duration<double, std::milli>(now - *last_tp).count();
            intervals_ms.push_back(dt);
        }
        last_tp = now;
    });

    try {
        client.connect();
        // Inicia o stream de GROSS (com fallback para 601A/01 em caso de erro)
        try {
            client.fetch(gross_path);
        } catch (const std::exception&) {
            const std::string fallback = "601A/01";
            std::cerr << "[warn] fetch falhou para path '" << gross_path
                      << "', tentando fallback '" << fallback << "'\n";
            gross_path = fallback;
            client.fetch(gross_path);
        }

    const auto t0 = SteadyClock::now();
        while (g_run.load()) {
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
            if (std::chrono::duration_cast<std::chrono::seconds>(SteadyClock::now() - t0).count() >= duration_sec) {
                break;
            }
        }
    } catch (const std::exception& ex) {
        std::cerr << "Erro: " << ex.what() << std::endl;
        client.disconnect();
        return 1;
    }

    client.disconnect();

    // Estatísticas
    double min_v = std::numeric_limits<double>::infinity();
    double max_v = 0.0;
    double sum_v = 0.0;
    size_t n = 0;
    {
        std::lock_guard<std::mutex> lk(mtx);
        n = intervals_ms.size();
        for (double v : intervals_ms) {
            if (v < min_v) min_v = v;
            if (v > max_v) max_v = v;
            sum_v += v;
        }
    }

    if (n == 0) {
        std::cout << "Nenhuma mensagem de GROSS capturada (verifique conexão/stream)." << std::endl;
        return 0;
    }

    const double avg_v = sum_v / static_cast<double>(n);

    // Percentis simples (p50/p90/p99)
    std::vector<double> sorted;
    {
        std::lock_guard<std::mutex> lk(mtx);
        sorted = intervals_ms;
    }
    std::sort(sorted.begin(), sorted.end());
    auto pct = [&](double p) {
        if (sorted.empty()) return 0.0;
        double idx = p * (sorted.size() - 1);
        size_t i = static_cast<size_t>(idx);
        size_t j = std::min(i + 1, sorted.size() - 1);
        double frac = idx - i;
        return sorted[i] * (1.0 - frac) + sorted[j] * frac;
    };

    std::cout << std::fixed << std::setprecision(3)
              << "Gross intervals (ms): count=" << n
              << ", min=" << min_v
              << ", avg=" << avg_v
              << ", max=" << max_v
              << ", p50=" << pct(0.50)
              << ", p90=" << pct(0.90)
              << ", p99=" << pct(0.99)
              << std::endl;

    return 0;
}
