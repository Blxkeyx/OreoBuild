#pragma once

#include "core/build_system.hpp"
#include <string>
#include <vector>
#include <chrono>

class CLIHandler {
public:
    CLIHandler(OreoBuild::BuildSystem& buildSystem);
    int run(int argc, char* argv[]);

    // Make these static methods public
    static void printUsage();
    static void printDetailedHelp();
    static bool isValidCommand(const std::string& cmd);

private:
    OreoBuild::BuildSystem& buildSystem;

    void parseArguments(const std::vector<std::string>& args);
    int executeCommand();

    void viewLog(const std::string& logFile);
    void cleanLog(const std::string& logFile, int days);
    void searchLog(const std::string& logFile, const std::string& searchTerm, bool caseInsensitive);
    void displayBuildType();
    void compareBuilds(const std::string& logFile, const std::string& id1, const std::string& id2);
    void listBuildIds(const std::string& logFile);
    void showProgress(int current, int total);
    void printBuildSummary(const std::string& target, std::chrono::microseconds duration);
    void appendBuildLog(const std::string& logFile, const std::string& target, 
                        std::chrono::microseconds duration, const std::string& buildSummary);
    std::string generateBuildId();

    bool handleLogCommands();
    bool isLogCommand() const;
    int executeBuildCommand();

    bool forceClean;
    OreoBuild::BuildSystem::VerbosityLevel verbosityLevel;
    std::string command;
    std::string target;
    std::string logFile;
    std::string viewLogFile;
    std::string cleanLogFile;
    int cleanLogDays;
    std::string searchLogFile;
    std::string searchTerm;
    bool caseInsensitiveSearch;
    std::string compareLogFile;
    std::string compareId1;
    std::string compareId2;
    bool listBuildIdsRequested;
    std::string buildTypeOverride;
};
