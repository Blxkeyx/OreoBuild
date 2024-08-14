#include "core/build_system.hpp"
#include "cli_handler.hpp"
#include "color.hpp"
#include <iostream>
#include <stdexcept>
#include <string>

int main(int argc, char* argv[]) {
    if (argc > 1 && std::string(argv[1]) == "--help") {
        CLIHandler::printDetailedHelp();
        return 0;
    }

    if (argc < 2) {
        CLIHandler::printUsage();
        return 1;
    }

    try {
        OreoBuild::BuildSystem buildSystem;
        
        // Check if the first argument is a valid command or option
        if (argv[1][0] == '-' || CLIHandler::isValidCommand(argv[1])) {
            std::cerr << Color::Red << "Error: Config file not specified." << Color::Reset << std::endl;
            CLIHandler::printUsage();
            return 1;
        }

        // Load config file
        try {
            buildSystem.loadConfig(argv[1]);
        } catch (const std::exception& e) {
            std::cerr << Color::Red << "Error loading config: " << e.what() << Color::Reset << std::endl;
            return 1;
        }

        CLIHandler cliHandler(buildSystem);
        return cliHandler.run(argc - 1, argv + 1);  // Skip the config file argument
    } catch (const std::exception& e) {
        std::cerr << Color::Red << "Error: " << e.what() << Color::Reset << std::endl;
        return 1;
    }
}
