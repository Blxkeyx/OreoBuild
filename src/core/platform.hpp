#pragma once
#include <string>
#include <memory>

namespace OreoBuild {

class Platform {
public:
    virtual ~Platform() = default;
    virtual std::string getName() const = 0;
    virtual std::string getPathSeparator() const = 0;
    virtual int execute(const std::string& command) = 0;
};

std::unique_ptr<Platform> createPlatform();

}
