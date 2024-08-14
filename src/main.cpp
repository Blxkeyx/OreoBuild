#include "core/build_system.hpp"
#include <iostream>
#include <stdexcept>

int main(int argc, char* argv[]) {
    if (argc < 2) {
        std::cout << "Usage: oreobuild <config_file> [clean|debug|release] [target]" << std::endl;
        return 1;
    }

    try {
        OreoBuild::BuildSystem buildSystem;
        buildSystem.loadConfig(argv[1]);

        if (argc > 2) {
            std::string command = argv[2];
            if (command == "clean") {
                buildSystem.clean();
                return 0;
            } else if (command == "debug") {
                buildSystem.getConfig().setBuildType(OreoBuild::BuildType::Debug);
                std::cout << "Build type set to Debug" << std::endl;
            } else if (command == "release") {
                buildSystem.getConfig().setBuildType(OreoBuild::BuildType::Release);
                std::cout << "Build type set to Release" << std::endl;
            }
        }

        std::string target = (argc > 3) ? argv[3] : "all";
        buildSystem.build(target);
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}
