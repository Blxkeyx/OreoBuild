#pragma once
#include "config.hpp"
#include "compiler.hpp"
#include "thread_pool.hpp"
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>
#include <set>
#include <fstream>
#include <chrono>

namespace OreoBuild {

class BuildSystem {
public:
    BuildSystem();
    ~BuildSystem();
    void loadConfig(const std::string& configFile);
    void build(const std::string& target, std::function<void(const std::string&)> progressCallback = nullptr);
    void clean(bool forceClean = false);
    const Config& getConfig() const { return config; }
    Config& getConfig() { return config; }
    std::string getBuildFlags() const;
    int getFilesCompiled() const { return filesCompiled; }

    enum class VerbosityLevel {
        Quiet,
        Normal,
        Verbose,
        VeryVerbose,
        ExtremelyVerbose
    };

    void setVerbosityLevel(VerbosityLevel level);

private:
    Config config;
    std::unique_ptr<Compiler> compiler;
    std::unordered_map<std::string, std::set<std::string>> dependencies;
    std::unique_ptr<ThreadPool> threadPool;
    std::string cacheFilePath;
    std::unordered_map<std::string, std::time_t> cacheMap;
    std::unordered_map<std::string, std::time_t> dependencyCacheTimestamps;

    bool needsRebuild(const std::string& source, const std::string& object);
    void parseDependencies(const std::string& source);
    std::vector<std::string> getObjectFiles() const; 
    void addDependency(const std::string& source, const std::string& dependency);
    void loadCache();
    void saveCache();
    VerbosityLevel verbosityLevel;
    std::chrono::high_resolution_clock::time_point buildStartTime;
    int filesCompiled;
    std::vector<std::string> splitString(const std::string& s, char delimiter);
    std::string joinString(const std::vector<std::string>& v, const std::string& delimiter);

};

}
