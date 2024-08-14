#include "build_system.hpp"
#include "file_utils.hpp"
#include "color.hpp"
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
      cacheFilePath("build_cache.txt"),
      verbosityLevel(VerbosityLevel::Normal),
      filesCompiled(0) {
    loadCache();
}

BuildSystem::~BuildSystem() {
    saveCache();
}

void BuildSystem::loadConfig(const std::string& configFile) {
    config.loadFromFile(configFile);

    // After loading, print out the contents
    if (verbosityLevel >= VerbosityLevel::Verbose) {
        std::cout << "Loaded configuration:" << std::endl;
        std::cout << "Compiler: " << config.getCompiler() << std::endl;
        std::cout << "Sources: " << joinString(config.getSourceFiles(), ", ") << std::endl;
        std::cout << "Output: " << config.getOutputFile() << std::endl;
        std::cout << "Include paths: " << joinString(config.getIncludePaths(), ", ") << std::endl;
        std::cout << "System Include paths: " << joinString(config.getSystemIncludePaths(), ", ") << std::endl;
        std::cout << "Libraries: " << joinString(config.getLibraries(), ", ") << std::endl;
        std::cout << "Build Type: " << (config.isDebug() ? "Debug" : "Release") << std::endl;
        std::cout << "Compiler Flags: " << joinString(config.getCompilerFlags(), " ") << std::endl;
        std::cout << "Debug Flags: " << config.getDebugFlags() << std::endl;
        std::cout << "Release Flags: " << config.getReleaseFlags() << std::endl;
    }
}

std::string OreoBuild::BuildSystem::joinString(const std::vector<std::string>& v, const std::string& delimiter) {
    std::string result;
    for (size_t i = 0; i < v.size(); ++i) {
        if (i > 0) {
            result += delimiter;
        }
        result += v[i];
    }
    return result;
}


void BuildSystem::loadCache() {
    std::ifstream cacheFile(cacheFilePath);
    if (cacheFile.is_open()) {
        std::string filePath;
        std::time_t timestamp;
        while (cacheFile >> filePath >> timestamp) {
            cacheMap[filePath] = timestamp;
        }
    }
}

void BuildSystem::saveCache() {
    std::ofstream cacheFile(cacheFilePath);
    if (cacheFile.is_open()) {
        for (const auto& entry : cacheMap) {
            cacheFile << entry.first << " " << entry.second << std::endl;
        }
    }
}

void BuildSystem::build(const std::string& target, std::function<void(const std::string&)> progressCallback) {
    buildStartTime = std::chrono::high_resolution_clock::now();
    filesCompiled = 0;

    if (verbosityLevel >= VerbosityLevel::Verbose) {
        std::cout << "Building target: " << target << std::endl;
        std::cout << "Build type: " << (config.getBuildType() == BuildType::Debug ? "Debug" : "Release") << std::endl;
        std::cout << "Using " << threadPool->getThreadCount() << " threads for compilation" << std::endl;
    }

    std::vector<std::string> objects;
    std::vector<std::string> objectsToCompile;
    std::mutex outputMutex;

    auto checkDependenciesStart = std::chrono::high_resolution_clock::now();

    for (const auto& source : config.getSourceFiles()) {
        if (!std::filesystem::exists(source)) {
            std::cerr << Color::Red << "Error: Source file not found: " << source << Color::Reset << std::endl;
            return;
        }
        std::string obj = std::filesystem::path(source).filename().replace_extension(".o").string();
        if (needsRebuild(source, obj)) {
            objectsToCompile.push_back(source);
        }
        objects.push_back(obj);
    }

    auto checkDependenciesEnd = std::chrono::high_resolution_clock::now();
    auto checkDependenciesDuration = std::chrono::duration_cast<std::chrono::milliseconds>(checkDependenciesEnd - checkDependenciesStart);

    if (verbosityLevel >= VerbosityLevel::VeryVerbose) {
        std::cout << "Time spent checking dependencies: " << checkDependenciesDuration.count() << " ms" << std::endl;
    }

    std::atomic<bool> compilationFailed(false);
    std::atomic<int> compiledCount(0);

    auto compilationStart = std::chrono::high_resolution_clock::now();

    size_t totalFiles = objectsToCompile.size();
    for (const auto& source : objectsToCompile) {
        threadPool->enqueue([this, &source, &outputMutex, &compilationFailed, &compiledCount, totalFiles, &progressCallback] {
            std::string obj = std::filesystem::path(source).filename().replace_extension(".o").string();
            if (compiler->compile(source, obj, config)) {
                {
                    std::lock_guard<std::mutex> lock(outputMutex);
                    if (verbosityLevel >= VerbosityLevel::Normal) {
                        std::cout << Color::Green << "Compiled: " << source << " to " << obj << Color::Reset << std::endl;
                    }
                    filesCompiled++;
                    compiledCount++;
                    if (progressCallback) {
                        progressCallback(source);
                    }
                }
                FileUtils::updateTimestamp(obj);

                std::time_t lastModified = std::filesystem::last_write_time(source).time_since_epoch().count();
                cacheMap[source] = lastModified;
            } else {
                {
                    std::lock_guard<std::mutex> lock(outputMutex);
                    std::cerr << Color::Red << "Failed to compile: " << source << Color::Reset << std::endl;
                }
                compilationFailed = true;
           }
        });
    }

    while (compiledCount < objectsToCompile.size() && !compilationFailed) {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }

    auto compilationEnd = std::chrono::high_resolution_clock::now();
    auto compilationDuration = std::chrono::duration_cast<std::chrono::milliseconds>(compilationEnd - compilationStart);

    if (verbosityLevel >= VerbosityLevel::Verbose) {
        std::cout << std::endl;  // New line after progress bar
    }

    if (compilationFailed) {
        std::cerr << Color::Red << "Build failed due to compilation errors." << Color::Reset << std::endl;
        return;
    }

    if (verbosityLevel >= VerbosityLevel::VeryVerbose) {
        std::cout << "Time spent on compilation: " << compilationDuration.count() << " ms" << std::endl;
    }

    auto linkingStart = std::chrono::high_resolution_clock::now();

    std::string output = config.getOutputFile();
    if (std::any_of(objects.begin(), objects.end(), 
                    [&output](const std::string& obj) { return FileUtils::isNewer(obj, output); })) {
        if (compiler->link(objects, output, config)) {
            if (verbosityLevel >= VerbosityLevel::Normal) {
                std::cout << Color::Green << "Build successful. Output: " << output << Color::Reset << std::endl;
            }
        } else {
            std::cerr << Color::Red << "Linking failed." << Color::Reset << std::endl;
        }
    } else {
        if (verbosityLevel >= VerbosityLevel::Normal) {
            std::cout << Color::Yellow << "Output is up to date. Skipping link step." << Color::Reset << std::endl;
        }
    }

    auto linkingEnd = std::chrono::high_resolution_clock::now();
    auto linkingDuration = std::chrono::duration_cast<std::chrono::milliseconds>(linkingEnd - linkingStart);

    if (verbosityLevel >= VerbosityLevel::VeryVerbose) {
        std::cout << "Time spent on linking: " << linkingDuration.count() << " ms" << std::endl;
    }
}

bool BuildSystem::needsRebuild(const std::string& source, const std::string& object) {
    if (verbosityLevel >= VerbosityLevel::Verbose) {
        std::cout << "Checking if " << source << " needs rebuild..." << std::endl;
        FileUtils::printFileInfo(source);
        FileUtils::printFileInfo(object);
    }

    static BuildType lastBuildType = BuildType::Debug;
    if (config.getBuildType() != lastBuildType) {
        lastBuildType = config.getBuildType();
        if (verbosityLevel >= VerbosityLevel::Verbose) std::cout << "Build type changed. Rebuilding." << std::endl;
        return true;
    }

    if (!std::filesystem::exists(object)) {
        if (verbosityLevel >= VerbosityLevel::Verbose) std::cout << "Object file doesn't exist. Rebuilding." << std::endl;
        return true;
    }

    std::time_t lastModified = std::filesystem::last_write_time(source).time_since_epoch().count();
    auto it = cacheMap.find(source);
    if (it == cacheMap.end() || lastModified > it->second) {
        if (verbosityLevel >= VerbosityLevel::Verbose) std::cout << "Source file is newer than cached timestamp. Rebuilding." << std::endl;
        return true;
    }

    parseDependencies(source);
    for (const auto& dep : dependencies[source]) {
        if (verbosityLevel >= VerbosityLevel::VeryVerbose) {
            std::cout << "Checking dependency: " << dep << std::endl;
            FileUtils::printFileInfo(dep);
        }
        if (!std::filesystem::exists(dep)) {
            std::cerr << "Warning: Dependency not found: " << dep << std::endl;
            continue;
        }
        if (FileUtils::isNewer(dep, object)) {
            if (verbosityLevel >= VerbosityLevel::Verbose) std::cout << "Dependency " << dep << " is newer than object file. Rebuilding." << std::endl;
            return true;
        }
    }

    if (verbosityLevel >= VerbosityLevel::Verbose) std::cout << source << " is up to date." << std::endl;
    return false;
}

void BuildSystem::parseDependencies(const std::string& source) {
    auto lastModified = std::filesystem::last_write_time(source).time_since_epoch().count();
    if (dependencyCacheTimestamps[source] == lastModified) {
        if (verbosityLevel >= VerbosityLevel::VeryVerbose) {
            std::cout << "Using cached dependencies for " << source << std::endl;
        }
        return;
    }

    if (verbosityLevel >= VerbosityLevel::VeryVerbose) std::cout << "Parsing dependencies for " << source << std::endl;
    if (dependencies.find(source) != dependencies.end()) {
        dependencies[source].clear();
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
                
                if (verbosityLevel >= VerbosityLevel::VeryVerbose) {
                    std::cout << "Found include: " << filename << (isSystemHeader ? " (system header)" : "") << std::endl;
                }
                
                if (!isSystemHeader) {
                    for (const auto& path : config.getIncludePaths()) {
                        std::string fullPath = std::filesystem::path(path) / filename;
                        if (std::filesystem::exists(fullPath)) {
                            if (verbosityLevel >= VerbosityLevel::Verbose) std::cout << "Adding dependency: " << fullPath << " for " << source << std::endl;
                            addDependency(source, fullPath);
                            break;
                        }
                    }
                }
            }
        }
    }

    dependencyCacheTimestamps[source] = lastModified;
    if (verbosityLevel >= VerbosityLevel::VeryVerbose) {
        std::cout << "Cached dependencies for " << source << std::endl;
    }
}

void BuildSystem::addDependency(const std::string& source, const std::string& dependency) {
    dependencies[source].insert(dependency);
    parseDependencies(dependency);
}

void BuildSystem::clean(bool forceClean) {
    if (!forceClean) {
        std::cout << "Are you sure you want to clean all build artifacts? This action cannot be undone. (y/N): ";
        std::string response;
        std::getline(std::cin, response);
        if (response != "y" && response != "Y") {
            std::cout << "Clean operation cancelled." << std::endl;
            return;
        }
    }

    std::cout << "Cleaning build artifacts..." << std::endl;
    
    int removedCount = 0;
    int failedCount = 0;

    auto removeFile = [this, &removedCount, &failedCount](const std::string& file) {
        try {
            if (std::filesystem::exists(file) && std::filesystem::remove(file)) {
                if (verbosityLevel >= VerbosityLevel::Verbose) std::cout << "Removed: " << file << std::endl;
                removedCount++;
            }
        } catch (const std::filesystem::filesystem_error& e) {
            std::cerr << "Error removing " << file << ": " << e.what() << std::endl;
            failedCount++;
        }
    };

    // Remove object files and output file
    for (const auto& obj : getObjectFiles()) {
        removeFile(obj);
    }
    removeFile(config.getOutputFile());
    
    // Clear cache file and map
    try {
        std::ofstream(cacheFilePath, std::ios::trunc).close();
        cacheMap.clear();
        dependencyCacheTimestamps.clear();
        if (verbosityLevel >= VerbosityLevel::Verbose) std::cout << "Cleared build cache" << std::endl;
    } catch (const std::exception& e) {
        std::cerr << "Error clearing cache: " << e.what() << std::endl;
        failedCount++;
    }
    
    std::cout << "Clean complete. Removed " << removedCount << " file(s)." << std::endl;
    if (failedCount > 0) {
        std::cout << "Failed to remove " << failedCount << " file(s)." << std::endl;
    }
}

std::vector<std::string> BuildSystem::getObjectFiles() const {
    std::vector<std::string> objects;
    objects.reserve(config.getSourceFiles().size());
    for (const auto& source : config.getSourceFiles()) {
        objects.push_back(std::filesystem::path(source).filename().replace_extension(".o").string());
    }
    return objects;
}

std::string BuildSystem::getBuildFlags() const {
    return config.getBuildType() == BuildType::Debug ? config.getDebugFlags() : config.getReleaseFlags();
}

void BuildSystem::setVerbosityLevel(VerbosityLevel level) {
    verbosityLevel = level;
}

}
