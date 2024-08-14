#pragma once
#include <string>
#include <vector>
#include <unordered_map>

namespace OreoBuild {

enum class BuildType {
    Debug,
    Release
};

class Config {
public:
    Config();
    void loadFromFile(const std::string& filename);
    
    std::string getCompiler() const { return get("compiler"); }
    std::vector<std::string> getSourceFiles() const { return getList("sources"); }
    std::string getOutputFile() const { return get("output"); }
    std::vector<std::string> getIncludePaths() const { return getList("include_paths"); }
    std::vector<std::string> getSystemIncludePaths() const { return systemIncludePaths; }
    std::vector<std::string> getLibraries() const { return getList("libraries", true); }
    bool isDebug() const { return buildType == BuildType::Debug; }
    BuildType getBuildType() const { return buildType; }
    void setBuildType(BuildType type) { buildType = type; }
    std::vector<std::string> getCompilerFlags() const;

private:
    std::unordered_map<std::string, std::string> data;
    std::vector<std::string> systemIncludePaths;
    BuildType buildType;
    
    std::string get(const std::string& key) const;
    std::vector<std::string> getList(const std::string& key, bool allowEmpty = false) const;
    void initializeSystemIncludePaths();
};

} // namespace OreoBuild
