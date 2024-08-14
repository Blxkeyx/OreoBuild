#pragma once
#include "config.hpp"
#include <string>
#include <vector>
#include <memory>

namespace OreoBuild {

class Compiler {
public:
    virtual ~Compiler() = default;
    virtual std::string getName() const = 0;
    virtual bool compile(const std::string& source, const std::string& output, const Config& config) = 0;
    virtual bool link(const std::vector<std::string>& objects, const std::string& output, const Config& config) = 0;
};

std::unique_ptr<Compiler> createCompiler(const std::string& name);

} // namespace OreoBuild
