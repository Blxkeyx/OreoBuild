#include "config.hpp"
#include <fstream>
#include <iostream>
#include <sstream>
#include <filesystem>
#include <algorithm>

namespace OreoBuild {

static std::string trim(const std::string& str) {
    auto start = std::find_if_not(str.begin(), str.end(), [](unsigned char c) { return std::isspace(c); });
    auto end = std::find_if_not(str.rbegin(), str.rend(), [](unsigned char c) { return std::isspace(c); }).base();
    return (start < end ? std::string(start, end) : std::string());
}

Config::Config() : buildType(BuildType::Debug) {
    initializeSystemIncludePaths();
    loadBuildType();
}

void Config::initializeSystemIncludePaths() {
    systemIncludePaths = {
        "/usr/include",
        "/usr/local/include",
        "/usr/include/c++/10"
    };
}

void Config::loadFromFile(const std::string& filename) {
    lastLoadedConfigFile = filename;
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
            key = trim(key);
            value = trim(value);
            set(key, value);
            std::cout << "Config: " << key << " = " << value << std::endl;
        }
    }

    // Only set build type from file if it hasn't been loaded from build_type.txt
    std::string debugValue = get("debug");
    if (!debugValue.empty()) {
        BuildType fileConfigBuildType = (debugValue == "true" ? BuildType::Debug : BuildType::Release);
        if (buildType != fileConfigBuildType) {
            std::cout << "Note: Build type in config file differs from saved build type. Using saved build type: "
                      << (buildType == BuildType::Debug ? "Debug" : "Release") << std::endl;
        }
    }

    // Load debug and release flags
    debugFlags = get("debug_flags", "-g -O0 -Wall -Wextra");
    releaseFlags = get("release_flags", "-O2 -DNDEBUG -march=native");
}

void Config::setBuildType(BuildType type) {
    buildType = type;
    saveBuildType();
    
    // Update the config file
    set("debug", (type == BuildType::Debug) ? "true" : "false");
    
    // Save the updated config to file
    std::ofstream configFile(lastLoadedConfigFile);
    if (configFile.is_open()) {
        for (const auto& entry : configEntries) {
            configFile << entry.first << " = " << entry.second << std::endl;
        }
    } else {
        std::cerr << "Warning: Unable to update config file." << std::endl;
    }
}

void Config::saveBuildType() const {
    std::ofstream file(buildTypeFile);
    if (file.is_open()) {
        file << (buildType == BuildType::Debug ? "Debug" : "Release");
    } else {
        std::cerr << "Warning: Unable to save build type to file: " << buildTypeFile << std::endl;
    }
}

void Config::loadBuildType() {
    std::ifstream file(buildTypeFile);
    if (file.is_open()) {
        std::string type;
        file >> type;
        buildType = (type == "Debug" ? BuildType::Debug : BuildType::Release);
    } else {
        std::cout << "No saved build type found. Using default (Debug)." << std::endl;
    }
}

std::string Config::get(const std::string& key, const std::string& defaultValue) const {
    for (const auto& entry : configEntries) {
        if (entry.first == key) {
            return entry.second;
        }
    }
    return defaultValue;
}

void Config::set(const std::string& key, const std::string& value) {
    for (auto& entry : configEntries) {
        if (entry.first == key) {
            entry.second = value;
            return;
        }
    }
    configEntries.emplace_back(key, value);
}

std::vector<std::string> Config::getList(const std::string& key, bool allowEmpty) const {
    std::vector<std::string> result;
    std::string value = get(key);
    if (value.empty() && !allowEmpty) {
        throw std::runtime_error("Configuration key not found: " + key);
    }
    std::istringstream iss(value);
    std::string item;
    while (std::getline(iss, item, ',')) {
        result.push_back(trim(item));
    }
    return result;
}

std::vector<std::string> Config::getCompilerFlags() const {
    std::vector<std::string> flags;
    flags.push_back("-std=c++17");  // Always use C++17
    
    std::string flagsStr = (buildType == BuildType::Debug) ? debugFlags : releaseFlags;
    std::istringstream iss(flagsStr);
    std::string flag;
    while (std::getline(iss, flag, ' ')) {
        if (!flag.empty()) {
            flags.push_back(flag);
        }
    }
    return flags;
}

std::string Config::getCompiler() const {
    std::string compiler = get("compiler");
    if (compiler == "gcc") {
        return "g++";  // Use g++ for C++ compilation
    }
    return compiler.empty() ? "g++" : compiler;
}

std::string Config::getDebugFlags() const {
    return debugFlags;
}

std::string Config::getReleaseFlags() const {
    return releaseFlags;
}

}
