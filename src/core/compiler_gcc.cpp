#include "compiler.hpp"
#include "config.hpp"
#include <iostream>
#include <sstream>
#include <cstdlib>

namespace OreoBuild {

class GCCCompiler : public Compiler {
public:
    std::string getName() const override { return "GCC"; }

    bool compile(const std::string& source, const std::string& output, const Config& config) override {
        std::ostringstream command;
        command << config.getCompiler() << " ";
        
        for (const auto& flag : config.getCompilerFlags()) {
            command << flag << " ";
        }
        
        for (const auto& path : config.getIncludePaths()) {
            command << "-I" << path << " ";
        }
        
        command << "-c " << source << " -o " << output;

        std::cout << "Compiling: " << source << " to " << output << std::endl;
        std::cout << "Command: " << command.str() << std::endl;
        
        int result = std::system(command.str().c_str());
        if (result != 0) {
            std::cerr << "Compilation failed with error code: " << result << std::endl;
        }
        return result == 0;
    }

    bool link(const std::vector<std::string>& objects, const std::string& output, const Config& config) override {
        std::ostringstream command;
        command << config.getCompiler() << " ";
        
        for (const auto& flag : config.getCompilerFlags()) {
            command << flag << " ";
        }
        
        for (const auto& obj : objects) {
            command << obj << " ";
        }
        
        command << "-o " << output << " ";
        
        for (const auto& lib : config.getLibraries()) {
            command << "-l" << lib << " ";
        }

        command << "-lstdc++ ";

        std::cout << "Linking: " << output << std::endl;
        std::cout << "Command: " << command.str() << std::endl;
        
        int result = std::system(command.str().c_str());
        if (result != 0) {
            std::cerr << "Linking failed with error code: " << result << std::endl;
        }
        return result == 0;
    }
};

std::unique_ptr<Compiler> createCompiler(const std::string& name) {
    if (name == "gcc" || name == "g++") {
        return std::make_unique<GCCCompiler>();
    }
    throw std::runtime_error("Unsupported compiler: " + name);
}

}
