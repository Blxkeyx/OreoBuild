#include "config.hpp"
#include <fstream>
#include <iostream>
#include <sstream>
#include <filesystem>

namespace OreoBuild {

Config::Config() : buildType(BuildType::Debug) {
    initializeSystemIncludePaths();
}

void Config::initializeSystemIncludePaths() {
    // Add default system include paths
    systemIncludePaths = {
        "/usr/include",
        "/usr/local/include",
        "/usr/include/c++/10" // Adjust version number as needed
    };
}

void Config::loadFromFile(const std::string& filename) {
    std::filesystem::path fullPath = std::filesystem::absolute(filename);
    std::cout << "Loading config file: " << fullPath << std::endl;

    std::ifstream file(fullPath);
    if (!file.is_open()) {
        throw std::runtime_error("Unable to open config file: " + fullPath.string());
    }

    std::string line;
    while (std::getline(file, line)) {
        std::istringstream iss(line);
        std::string key, value;
        if (std::getline(iss, key, '=') && std::getline(iss, value)) {
            data[key] = value;
            std::cout << "Config: " << key << " = " << value << std::endl;
        }
    }

    // After loading the file, set the build type based on the 'debug' flag
    if (get("debug") == "true") {
        buildType = BuildType::Debug;
    } else {
        buildType = BuildType::Release;
    }
}

std::string Config::get(const std::string& key) const {
    auto it = data.find(key);
    if (it == data.end()) {
        throw std::runtime_error("Configuration key not found: " + key);
    }
    return it->second;
}

std::vector<std::string> Config::getList(const std::string& key, bool allowEmpty) const {
    std::vector<std::string> result;
    auto it = data.find(key);
    if (it == data.end()) {
        if (allowEmpty) {
            return result;  // Return empty vector
        }
        throw std::runtime_error("Configuration key not found: " + key);
    }
    std::istringstream iss(it->second);
    std::string item;
    while (std::getline(iss, item, ',')) {
        result.push_back(item);
    }
    return result;
}

std::vector<std::string> Config::getCompilerFlags() const {
    std::vector<std::string> flags;
    if (buildType == BuildType::Debug) {
        flags.push_back("-g");
        flags.push_back("-O0");
    } else {
        flags.push_back("-O2");
    }
    return flags;
}

} // namespace OreoBuild
