#include "dependency_manager.hpp"

namespace OreoBuild {

void DependencyManager::addDependency(const std::string& target, const std::string& dependency) {
    dependencies[target].push_back(dependency);
}

std::vector<std::string> DependencyManager::getBuildOrder(const std::string& target) {
    std::vector<std::string> order;
    std::unordered_set<std::string> visited;
    dfs(target, order, visited);
    return order;
}

void DependencyManager::dfs(const std::string& target, std::vector<std::string>& order, std::unordered_set<std::string>& visited) {
    if (visited.find(target) != visited.end()) {
        return;
    }
    visited.insert(target);
    for (const auto& dep : dependencies[target]) {
        dfs(dep, order, visited);
    }
    order.push_back(target);
}

} // namespace OreoBuild
