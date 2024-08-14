#include "build_system.hpp"
#include "file_utils.hpp"
#include <iostream>
#include <fstream>
#include <sstream>
#include <filesystem>
#include <algorithm>
#include <future>
#include <chrono>
#include <thread>

namespace OreoBuild {

BuildSystem::BuildSystem() 
    : compiler(createCompiler("gcc")),
      threadPool(std::make_unique<ThreadPool>(std::thread::hardware_concurrency())),
      cacheFilePath("build_cache.txt") {
    loadCache();
}

BuildSystem::~BuildSystem() {
    saveCache();
}

void BuildSystem::loadConfig(const std::string& configFile) {
    config.loadFromFile(configFile);
}

void BuildSystem::loadCache() {
    std::ifstream cacheFile(cacheFilePath);
    if (cacheFile.is_open()) {
        std::string line;
        while (std::getline(cacheFile, line)) {
            std::istringstream iss(line);
            std::string sourceFile;
            std::time_t lastModified;
            if (iss >> sourceFile >> lastModified) {
                cacheMap[sourceFile] = lastModified;
            }
        }
        cacheFile.close();
    }
}

void BuildSystem::saveCache() {
    std::ofstream cacheFile(cacheFilePath);
    if (cacheFile.is_open()) {
        for (const auto& entry : cacheMap) {
            cacheFile << entry.first << " " << entry.second << std::endl;
        }
        cacheFile.close();
    }
}

void BuildSystem::build(const std::string& target) {
    std::cout << "Building target: " << target << std::endl;

    std::vector<std::string> objects;
    std::vector<std::string> objectsToCompile;
    std::mutex outputMutex;

    for (const auto& source : config.getSourceFiles()) {
        if (!std::filesystem::exists(source)) {
            std::cerr << "Error: Source file not found: " << source << std::endl;
            return;
        }
        std::string obj = std::filesystem::path(source).filename().replace_extension(".o").string();
        if (needsRebuild(source, obj)) {
            objectsToCompile.push_back(source);
        }
        objects.push_back(obj);
    }

    std::atomic<bool> compilationFailed(false);
    std::atomic<int> compiledCount(0);

    for (const auto& source : objectsToCompile) {
        threadPool->enqueue([this, source, &outputMutex, &compilationFailed, &compiledCount] {
            std::string obj = std::filesystem::path(source).filename().replace_extension(".o").string();
            if (compiler->compile(source, obj, config)) {
                {
                    std::lock_guard<std::mutex> lock(outputMutex);
                    std::cout << "Compiled: " << source << " to " << obj << std::endl;
                }
                FileUtils::updateTimestamp(obj);
                compiledCount++;

                // Update cache after successful compilation
                std::time_t lastModified = std::filesystem::last_write_time(source).time_since_epoch().count();
                cacheMap[source] = lastModified;
            } else {
                {
                    std::lock_guard<std::mutex> lock(outputMutex);
                    std::cerr << "Failed to compile: " << source << std::endl;
                }
                compilationFailed = true;
            }
        });
    }

    // Wait for all compilations to complete
    while (compiledCount < objectsToCompile.size() && !compilationFailed) {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }

    if (compilationFailed) {
        std::cerr << "Build failed due to compilation errors." << std::endl;
        return;
    }

    std::string output = config.getOutputFile();
    if (std::any_of(objects.begin(), objects.end(), 
                    [&output](const std::string& obj) { return FileUtils::isNewer(obj, output); })) {
        if (compiler->link(objects, output, config)) {
            std::cout << "Build successful. Output: " << output << std::endl;
        } else {
            std::cerr << "Linking failed." << std::endl;
        }
    } else {
        std::cout << "Output is up to date. Skipping link step." << std::endl;
    }
}

bool BuildSystem::needsRebuild(const std::string& source, const std::string& object) {
    std::cout << "Checking if " << source << " needs rebuild..." << std::endl;
    FileUtils::printFileInfo(source);
    FileUtils::printFileInfo(object);

    static BuildType lastBuildType = BuildType::Debug;
    if (config.getBuildType() != lastBuildType) {
        lastBuildType = config.getBuildType();
        std::cout << "Build type changed. Rebuilding." << std::endl;
        return true;
    }

    if (!std::filesystem::exists(object)) {
        std::cout << "Object file doesn't exist. Rebuilding." << std::endl;
        return true;
    }

    std::time_t lastModified = std::filesystem::last_write_time(source).time_since_epoch().count();
    auto it = cacheMap.find(source);
    if (it == cacheMap.end() || lastModified > it->second) {
        std::cout << "Source file is newer than cached timestamp. Rebuilding." << std::endl;
        return true;
    }

    parseDependencies(source);
    for (const auto& dep : dependencies[source]) {
        std::cout << "Checking dependency: " << dep << std::endl;
        FileUtils::printFileInfo(dep);
        if (!std::filesystem::exists(dep)) {
            std::cerr << "Warning: Dependency not found: " << dep << std::endl;
            continue;
        }
        if (FileUtils::isNewer(dep, object)) {
            std::cout << "Dependency " << dep << " is newer than object file. Rebuilding." << std::endl;
            return true;
        }
    }

    std::cout << source << " is up to date." << std::endl;
    return false;
}

void BuildSystem::parseDependencies(const std::string& source) {
    std::cout << "Parsing dependencies for " << source << std::endl;
    if (dependencies.find(source) != dependencies.end()) {
        std::cout << "Dependencies for " << source << " already parsed." << std::endl;
        return;
    }

    std::ifstream file(source);
    std::string line;
    while (std::getline(file, line)) {
        if (line.find("#include") != std::string::npos) {
            std::istringstream iss(line);
            std::string include, filename;
            iss >> include >> filename;
            if (!filename.empty()) {
                bool isSystemHeader = filename[0] == '<';
                filename = filename.substr(1, filename.length() - 2);
                
                if (!isSystemHeader) {
                    for (const auto& path : config.getIncludePaths()) {
                        std::string fullPath = std::filesystem::path(path) / filename;
                        if (std::filesystem::exists(fullPath)) {
                            std::cout << "Adding dependency: " << fullPath << " for " << source << std::endl;
                            addDependency(source, fullPath);
                            break;
                        }
                    }
                }
            }
        }
    }
}

void BuildSystem::addDependency(const std::string& source, const std::string& dependency) {
    dependencies[source].insert(dependency);
    parseDependencies(dependency);
}

void BuildSystem::clean() {
    std::cout << "Cleaning build artifacts..." << std::endl;
    
    for (const auto& obj : getObjectFiles()) {
        if (std::filesystem::exists(obj)) {
            std::filesystem::remove(obj);
            std::cout << "Removed: " << obj << std::endl;
        }
    }
    
    std::string output = config.getOutputFile();
    if (std::filesystem::exists(output)) {
        std::filesystem::remove(output);
        std::cout << "Removed: " << output << std::endl;
    }
    
    // Clear the cache file
    std::ofstream cacheFile(cacheFilePath, std::ios::trunc);
    cacheFile.close();
    cacheMap.clear();
    
    std::cout << "Clean complete." << std::endl;
}

std::vector<std::string> BuildSystem::getObjectFiles() {
    std::vector<std::string> objects;
    for (const auto& source : config.getSourceFiles()) {
        std::string obj = std::filesystem::path(source).filename().replace_extension(".o").string();
        objects.push_back(obj);
    }
    return objects;
}

} // namespace OreoBuild
