#pragma once
#include "config.hpp"
#include "compiler.hpp"
#include "thread_pool.hpp"
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>
#include <set>

namespace OreoBuild {

class BuildSystem {
public:
    BuildSystem();
    ~BuildSystem();
    void loadConfig(const std::string& configFile);
    void build(const std::string& target);
    void clean();
    Config& getConfig() { return config; }

private:
    Config config;
    std::unique_ptr<Compiler> compiler;
    std::unordered_map<std::string, std::set<std::string>> dependencies;
    std::unique_ptr<ThreadPool> threadPool;

    bool needsRebuild(const std::string& source, const std::string& object);
    void parseDependencies(const std::string& source);
    std::vector<std::string> getObjectFiles();
    void addDependency(const std::string& source, const std::string& dependency);
};

} // namespace OreoBuild
