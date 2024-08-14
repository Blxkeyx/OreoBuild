#pragma once
#include <string>
#include <vector>
#include <unordered_map>
#include <unordered_set>

namespace OreoBuild {

class DependencyManager {
public:
    void addDependency(const std::string& target, const std::string& dependency);
    std::vector<std::string> getBuildOrder(const std::string& target);

private:
    std::unordered_map<std::string, std::vector<std::string>> dependencies;
    void dfs(const std::string& target, std::vector<std::string>& order, std::unordered_set<std::string>& visited);
};

}
