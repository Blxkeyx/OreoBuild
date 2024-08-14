#include "platform.hpp"
#include <cstdlib>

namespace OreoBuild {

class UnixPlatform : public Platform {
public:
    std::string getName() const override { return "Unix"; }
    std::string getPathSeparator() const override { return "/"; }
    int execute(const std::string& command) override {
        return std::system(command.c_str());
    }
};

std::unique_ptr<Platform> createPlatform() {
    return std::make_unique<UnixPlatform>();
}

} // namespace OreoBuild
