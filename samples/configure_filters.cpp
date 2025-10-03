#include "dse/dse_jet.hpp"

#include <chrono>
#include <iostream>
#include <cstdint>
#include <string>

int main(int argc, char** argv) {
    if (argc < 2) {
        std::cerr << "Uso: " << argv[0] << " <ip> [porta]\n";
        return 1;
    }

    const std::string host = argv[1];
    const std::uint16_t port = (argc > 2) ? static_cast<std::uint16_t>(std::stoi(argv[2])) : 80;

    try {
        dse::JetConnection connection(host, port, std::chrono::milliseconds{5000});
        dse::DSEJet dse(std::move(connection));
        dse.connect();

        std::cout << "Conectado ao DSE\n";
        std::cout << "Número de série: " << dse.serial_number() << "\n";
        std::cout << "Identificação: " << dse.identification() << "\n";
        std::cout << "Firmware: " << dse.firmware_version() << "\n";

        dse::FilterConfiguration stage2;
        stage2.mode = dse::FilterMode::FirComb;
        stage2.cutoff_frequency = 1000;
        dse.apply_filter_configuration(dse::FilterStage::Stage2, stage2);

        auto current = dse.filter_configuration(dse::FilterStage::Stage2);
        std::cout << "Filtro estágio 2 configurado para modo "
                  << static_cast<int>(current.mode)
                  << " com frequência de corte " << current.cutoff_frequency << " mHz\n";

        dse.disconnect();
    } catch (const std::exception& ex) {
        std::cerr << "Erro ao comunicar com DSE: " << ex.what() << "\n";
        return 1;
    }

    return 0;
}
