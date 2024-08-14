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
    
    bool isInitialized() const {
        return !getCompiler().empty() && !getSourceFiles().empty() && !getOutputFile().empty();
    }
    
    std::string getCompiler() const;
    std::vector<std::string> getSourceFiles() const { return getList("sources"); }
    std::string getOutputFile() const { return get("output", "a.out"); }
    std::vector<std::string> getIncludePaths() const { return getList("include_paths"); }
    std::vector<std::string> getSystemIncludePaths() const { return systemIncludePaths; }
    std::vector<std::string> getLibraries() const { return getList("libraries", true); }
    bool isDebug() const { return buildType == BuildType::Debug; }
    BuildType getBuildType() const { return buildType; }
    void setBuildType(BuildType type);
    std::vector<std::string> getCompilerFlags() const;
    std::string getDebugFlags() const;
    std::string getReleaseFlags() const;

    void saveBuildType() const;
    void loadBuildType();

private:
    std::vector<std::pair<std::string, std::string>> configEntries;
    std::vector<std::string> systemIncludePaths;
    BuildType buildType;
    std::string debugFlags;
    std::string releaseFlags;
    std::string lastLoadedConfigFile;
    
    std::string get(const std::string& key, const std::string& defaultValue = "") const;
    void set(const std::string& key, const std::string& value);
    std::vector<std::string> getList(const std::string& key, bool allowEmpty = false) const;
    void initializeSystemIncludePaths();

    const std::string buildTypeFile = "build_type.txt";
};

} // namespace OreoBuild
