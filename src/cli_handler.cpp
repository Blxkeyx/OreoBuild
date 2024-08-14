#include "cli_handler.hpp"
#include "color.hpp"
#include <iostream>
#include <fstream>
#include <iomanip>
#include <regex>
#include <algorithm>
#include <ctime>
#include <sstream>

CLIHandler::CLIHandler(OreoBuild::BuildSystem& bs) : buildSystem(bs) {
    forceClean = false;
    verbosityLevel = OreoBuild::BuildSystem::VerbosityLevel::Normal;
    target = "all";
    cleanLogDays = 0;
    caseInsensitiveSearch = false;
    listBuildIdsRequested = false;
}

int CLIHandler::run(int argc, char* argv[]) {
    if (argc > 0 && std::string(argv[0]) == "--help") {
        printDetailedHelp();
        return 0;
    }

    if (argc < 1) {
        printUsage();
        return 1;
    }

    parseArguments(std::vector<std::string>(argv, argv + argc));

    return executeCommand();
}

bool CLIHandler::isValidCommand(const std::string& cmd) {
    return cmd == "build" || cmd == "clean" || cmd == "debug" || cmd == "release" || cmd == "build-type";
}

void CLIHandler::parseArguments(const std::vector<std::string>& args) {
    for (const auto& arg : args) {
        if (arg == "--help") {
            command = "--help";
            return;  // Exit parsing immediately if --help is found
        } else if (arg == "--force") {
            forceClean = true;
        } else if (arg == "-v") {
            verbosityLevel = OreoBuild::BuildSystem::VerbosityLevel::Verbose;
        } else if (arg == "-vv") {
            verbosityLevel = OreoBuild::BuildSystem::VerbosityLevel::VeryVerbose;
        } else if (arg == "-vvv") {
            verbosityLevel = OreoBuild::BuildSystem::VerbosityLevel::ExtremelyVerbose;
        } else if (arg.substr(0, 6) == "--log=") {
            logFile = arg.substr(6);
        } else if (arg.substr(0, 11) == "--view-log=") {
            viewLogFile = arg.substr(11);
        } else if (arg.substr(0, 12) == "--clean-log=") {
            size_t colonPos = arg.find(':', 12);
            if (colonPos != std::string::npos) {
                cleanLogFile = arg.substr(12, colonPos - 12);
                cleanLogDays = std::stoi(arg.substr(colonPos + 1));
            }
        } else if (arg.substr(0, 13) == "--search-log=") {
            size_t colonPos = arg.find(':', 13);
            if (colonPos != std::string::npos) {
                searchLogFile = arg.substr(13, colonPos - 13);
                searchTerm = arg.substr(colonPos + 1);
            }
        } else if (arg == "--case-insensitive") {
            caseInsensitiveSearch = true;
        } else if (arg.substr(0, 17) == "--compare-builds=") {
            size_t colonPos1 = arg.find(':', 17);
            size_t colonPos2 = arg.find(':', colonPos1 + 1);
            if (colonPos1 != std::string::npos && colonPos2 != std::string::npos) {
                compareLogFile = arg.substr(17, colonPos1 - 17);
                compareId1 = arg.substr(colonPos1 + 1, colonPos2 - colonPos1 - 1);
                compareId2 = arg.substr(colonPos2 + 1);
            }
        } else if (arg == "--list-build-ids") {
            listBuildIdsRequested = true;
        } else if (command.empty()) {
            command = arg;
        } else if (target == "all") {
            target = arg;
        }
    }
}

int CLIHandler::executeCommand() {
    if (command == "--help") {
        printDetailedHelp();
        return 0;
    }

    buildSystem.setVerbosityLevel(verbosityLevel);

    // Handle log-related commands
    if (handleLogCommands()) {
        return 0;
    }

    // Check for config initialization if not handling log-related commands
    if (!isLogCommand()) {
        if (!buildSystem.getConfig().isInitialized()) {
            std::cerr << Color::Red << "Error: Configuration not properly loaded. Please check your config file." << Color::Reset << std::endl;
            return 1;
        }

        if (buildSystem.getConfig().getSourceFiles().empty()) {
            std::cerr << Color::Red << "Error: Configuration not loaded or no source files specified. Please check your config file." << Color::Reset << std::endl;
            return 1;
        }
    }
    
    if (command == "clean") {
        buildSystem.clean(forceClean);
        return 0;
    } else if (command == "debug") {
        buildSystem.getConfig().setBuildType(OreoBuild::BuildType::Debug);
        std::cout << Color::Green << "Build type set to Debug" << Color::Reset << std::endl;
        return 0;
    } else if (command == "release") {
        buildSystem.getConfig().setBuildType(OreoBuild::BuildType::Release);
        std::cout << Color::Green << "Build type set to Release" << Color::Reset << std::endl;
        return 0;
    } else if (command == "build-type") {
        displayBuildType();
        return 0;
    } else if (command.empty() || command == "build") {
        return executeBuildCommand();
    } else {
        std::cerr << Color::Red << "Unknown command: " << command << Color::Reset << std::endl;
        printUsage();
        return 1;
    }

    return 0;
}

bool CLIHandler::handleLogCommands() {
    if (listBuildIdsRequested) {
        if (!logFile.empty()) {
            listBuildIds(logFile);
        } else {
            std::cerr << Color::Red << "Error: Log file not specified. Use --log=<file> to specify a log file." << Color::Reset << std::endl;
        }
        return true;
    }

    if (!viewLogFile.empty()) {
        viewLog(viewLogFile);
        return true;
    }

    if (!cleanLogFile.empty() && cleanLogDays > 0) {
        cleanLog(cleanLogFile, cleanLogDays);
        return true;
    }

    if (!searchLogFile.empty() && !searchTerm.empty()) {
        searchLog(searchLogFile, searchTerm, caseInsensitiveSearch);
        return true;
    }

    if (!compareLogFile.empty() && !compareId1.empty() && !compareId2.empty()) {
        compareBuilds(compareLogFile, compareId1, compareId2);
        return true;
    }

    return false;
}

bool CLIHandler::isLogCommand() const {
    return listBuildIdsRequested || !viewLogFile.empty() || !cleanLogFile.empty() || 
           !searchLogFile.empty() || !compareLogFile.empty();
}

int CLIHandler::executeBuildCommand() {
    auto buildType = buildSystem.getConfig().getBuildType();
    std::cout << Color::Cyan << "Building in " << (buildType == OreoBuild::BuildType::Debug ? "Debug" : "Release") << " mode" << Color::Reset << std::endl;
    
    if (!buildTypeOverride.empty()) {
        if (buildTypeOverride == "debug") {
            buildSystem.getConfig().setBuildType(OreoBuild::BuildType::Debug);
        } else if (buildTypeOverride == "release") {
            buildSystem.getConfig().setBuildType(OreoBuild::BuildType::Release);
        }
        std::cout << Color::Yellow << "Build type overridden to: " << buildTypeOverride << Color::Reset << std::endl;
    }

    auto startTime = std::chrono::high_resolution_clock::now();

    std::string buildSummary;
    int totalFiles = buildSystem.getConfig().getSourceFiles().size();
    int compiledFiles = 0;

    try {
        buildSystem.build(target, [&](const std::string& file) {
            compiledFiles++;
            if (verbosityLevel >= OreoBuild::BuildSystem::VerbosityLevel::Verbose) {
                showProgress(compiledFiles, totalFiles);
            }
        });

        if (compiledFiles > 0) {
            buildSummary = "Compiled " + std::to_string(compiledFiles) + " file(s).";
        } else {
            buildSummary = "All files up to date. No compilation needed.";
        }

        auto endTime = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::microseconds>(endTime - startTime);

        if (verbosityLevel >= OreoBuild::BuildSystem::VerbosityLevel::Verbose) {
            std::cout << std::endl;  // New line after progress bar
            printBuildSummary(target, duration);
        } else {
            std::cout << Color::Green << "Build completed successfully in " 
                      << std::fixed << std::setprecision(3) << duration.count() / 1000000.0 
                      << " seconds. " << buildSummary << Color::Reset << std::endl;
        }

        if (!logFile.empty()) {
            appendBuildLog(logFile, target, duration, buildSummary);
        }
    } catch (const std::exception& e) {
        std::cerr << Color::Red << "Build failed: " << e.what() << Color::Reset << std::endl;
        return 1;
    }

    return 0;
}


void CLIHandler::displayBuildType() {
    auto buildType = buildSystem.getConfig().getBuildType();
    std::cout << "Current build type: " << (buildType == OreoBuild::BuildType::Debug ? "Debug" : "Release") << std::endl;
}

void CLIHandler::printUsage() {
    std::cout << "Usage: oreobuild <config_file> [command] [options]" << std::endl;
    std::cout << "Type 'oreobuild --help' for more information." << std::endl;
}

void CLIHandler::printDetailedHelp() {
    std::cout << "OreoBuilder - An efficient build system" << std::endl;
    std::cout << std::endl;
    std::cout << "USAGE:" << std::endl;
    std::cout << "  oreobuild <config_file> [command] [options]" << std::endl;
    std::cout << std::endl;
    std::cout << "COMMANDS:" << std::endl;
    std::cout << "  build [target]    Build the specified target or all targets if not specified" << std::endl;
    std::cout << "  clean             Clean build artifacts" << std::endl;
    std::cout << "  debug             Set build type to Debug" << std::endl;
    std::cout << "  release           Set build type to Release" << std::endl;
    std::cout << "  build-type        Display the current build type" << std::endl;
    std::cout << std::endl;
    std::cout << "OPTIONS:" << std::endl;
    std::cout << "  --force           Force clean without confirmation" << std::endl;
    std::cout << "  -v, -vv, -vvv     Set verbosity level (verbose, more verbose, very verbose)" << std::endl;
    std::cout << "  --log=<file>      Append build log to specified file" << std::endl;
    std::cout << "  --view-log=<file> View the contents of the specified log file" << std::endl;
    std::cout << "  --clean-log=<file>:<days>  Remove log entries older than <days> days" << std::endl;
    std::cout << "  --search-log=<file>:<term> Search log file for entries containing <term>" << std::endl;
    std::cout << "  --case-insensitive          Use case-insensitive search with --search-log" << std::endl;
    std::cout << "  --compare-builds=<file>:<id1>:<id2>  Compare two builds by their IDs" << std::endl;
    std::cout << "  --list-build-ids --log=<file>  List all available build IDs in the log file" << std::endl;
    std::cout << "  --help            Display this help message" << std::endl;
    std::cout << std::endl;
    std::cout << "EXAMPLES:" << std::endl;
    std::cout << "  oreobuild config.txt build" << std::endl;
    std::cout << "  oreobuild config.txt clean --force" << std::endl;
    std::cout << "  oreobuild config.txt build -vv --log=build.log" << std::endl;
    std::cout << "  oreobuild config.txt --search-log=build.log:error --case-insensitive" << std::endl;
    std::cout << "  oreobuild config.txt --compare-builds=build.log:220240814_143515:20240814_144326" << std::endl;
}

void CLIHandler::viewLog(const std::string& logFile) {
    std::ifstream log(logFile);
    if (log.is_open()) {
        std::cout << Color::Blue << "Contents of " << logFile << ":" << Color::Reset << std::endl;
        std::cout << std::string(40, '-') << std::endl;
        std::string line;
        while (std::getline(log, line)) {
            std::cout << line << std::endl;
        }
        std::cout << std::string(40, '-') << std::endl;
        log.close();
    } else {
        std::cerr << Color::Red << "Failed to open log file: " << logFile << Color::Reset << std::endl;
    }
}

void CLIHandler::cleanLog(const std::string& logFile, int days) {
    std::ifstream inFile(logFile);
    std::vector<std::string> entries;
    std::string line;
    std::string currentEntry;
    auto now = std::chrono::system_clock::now();
    auto cutoff = now - std::chrono::hours(24 * days);

    while (std::getline(inFile, line)) {
        if (line.find("--- Build Log Entry") != std::string::npos) {
            if (!currentEntry.empty()) {
                entries.push_back(currentEntry);
            }
            currentEntry = line + "\n";
        } else {
            currentEntry += line + "\n";
        }
    }
    if (!currentEntry.empty()) {
        entries.push_back(currentEntry);
    }
    inFile.close();

    std::ofstream outFile(logFile);
    for (const auto& entry : entries) {
        std::istringstream iss(entry);
        std::string dateLine;
        std::getline(iss, dateLine);
        std::getline(iss, dateLine);
        std::tm tm = {};
        std::istringstream dateStream(dateLine.substr(6));
        dateStream >> std::get_time(&tm, "%Y-%m-%d %H:%M:%S");
        auto entryTime = std::chrono::system_clock::from_time_t(std::mktime(&tm));
        if (entryTime >= cutoff) {
            outFile << entry;
        }
    }
    outFile.close();

    std::cout << Color::Green << "Log file cleaned. Entries older than " << days << " days have been removed." << Color::Reset << std::endl;
}

void CLIHandler::searchLog(const std::string& logFile, const std::string& searchTerm, bool caseInsensitive) {
    std::ifstream log(logFile);
    if (log.is_open()) {
        std::cout << Color::Blue << "Searching for \"" << searchTerm << "\" in " << logFile << ":" << Color::Reset << std::endl;
        std::string line;
        std::string currentBuildId;
        bool inMatchingEntry = false;
        std::regex searchRegex(searchTerm, caseInsensitive ? std::regex_constants::icase : std::regex_constants::ECMAScript);

        while (std::getline(log, line)) {
            if (line.find("--- Build Log Entry (ID: ") != std::string::npos) {
                currentBuildId = line.substr(line.find("ID: ") + 4, 15);
                inMatchingEntry = false;
            }
            if (std::regex_search(line, searchRegex)) {
                if (!inMatchingEntry) {
                    std::cout << Color::Yellow << "\nBuild ID: " << currentBuildId << Color::Reset << std::endl;
                }
                inMatchingEntry = true;
                std::cout << line << std::endl;
            }
        }
        log.close();
    } else {
        std::cerr << Color::Red << "Failed to open log file: " << logFile << Color::Reset << std::endl;
    }
}

void CLIHandler::compareBuilds(const std::string& logFile, const std::string& id1, const std::string& id2) {
    std::ifstream log(logFile);
    if (log.is_open()) {
        std::string line;
        std::map<std::string, std::map<std::string, std::string>> builds;
        std::string currentId;
        while (std::getline(log, line)) {
            if (line.find("--- Build Log Entry (ID: ") != std::string::npos) {
                currentId = line.substr(line.find("ID: ") + 4, 15);
            }
            if (currentId == id1 || currentId == id2) {
                std::istringstream iss(line);
                std::string key, value;
                if (std::getline(iss, key, ':') && std::getline(iss, value)) {
                    builds[currentId][key] = value;
                }
            }
        }
        log.close();

        if (builds.find(id1) == builds.end() || builds.find(id2) == builds.end()) {
            std::cerr << Color::Red << "One or both build IDs not found in log file." << Color::Reset << std::endl;
            return;
        }

        std::cout << Color::Blue << "Comparing builds " << id1 << " and " << id2 << ":" << Color::Reset << std::endl;
        for (const auto& [key, value1] : builds[id1]) {
            if (builds[id2].find(key) != builds[id2].end()) {
                if (value1 != builds[id2][key]) {
                    std::cout << key << ":" << std::endl;
                    std::cout << "  " << id1 << ": " << value1 << std::endl;
                    std::cout << "  " << id2 << ": " << builds[id2][key] << std::endl;
                }
            }
        }
    } else {
        std::cerr << Color::Red << "Failed to open log file: " << logFile << Color::Reset << std::endl;
    }
}

void CLIHandler::listBuildIds(const std::string& logFile) {
    std::ifstream log(logFile);
    if (log.is_open()) {
        std::cout << Color::Blue << "Available Build IDs in " << logFile << ":" << Color::Reset << std::endl;
        std::string line;
        bool foundBuildIds = false;
        while (std::getline(log, line)) {
            if (line.find("--- Build Log Entry (ID: ") != std::string::npos) {
                std::cout << line.substr(line.find("ID: ") + 4, 15) << std::endl;
                foundBuildIds = true;
            }
        }
        if (!foundBuildIds) {
            std::cout << Color::Yellow << "No Build IDs found in the log file." << Color::Reset << std::endl;
        }
        log.close();
    } else {
        std::cerr << Color::Red << "Failed to open log file: " << logFile << Color::Reset << std::endl;
    }
}

void CLIHandler::showProgress(int current, int total) {
    int barWidth = 50;
    float progress = (float)current / total;
    int pos = barWidth * progress;
    std::cout << "[";
    for (int i = 0; i < barWidth; ++i) {
        if (i < pos) std::cout << "=";
        else if (i == pos) std::cout << ">";
        else std::cout << " ";
    }
    std::cout << "] " << int(progress * 100.0) << " %\r";
    std::cout.flush();
}

void CLIHandler::printBuildSummary(const std::string& target, std::chrono::microseconds duration) {
    std::cout << Color::Green << "\nBuild Summary:" << Color::Reset << std::endl;
    std::cout << "  Total time: " << duration.count() << " µs (" 
              << std::fixed << std::setprecision(3) << duration.count() / 1000000.0 << " seconds)" << std::endl;
    std::cout << "  Target: " << target << std::endl;
    std::cout << "  Build type: " << (buildSystem.getConfig().getBuildType() == OreoBuild::BuildType::Debug ? "Debug" : "Release") << std::endl;
    std::cout << "  Files compiled: " << buildSystem.getFilesCompiled() << std::endl;
    std::cout << "  Up-to-date files: " << (buildSystem.getConfig().getSourceFiles().size() - buildSystem.getFilesCompiled()) << std::endl;
}

void CLIHandler::appendBuildLog(const std::string& logFile, const std::string& target, 
                                std::chrono::microseconds duration, const std::string& buildSummary) {
    std::ofstream log(logFile, std::ios::app);  // Open in append mode
    if (log.is_open()) {
        std::string buildId = generateBuildId();
        auto now = std::chrono::system_clock::now();
        auto in_time_t = std::chrono::system_clock::to_time_t(now);

        log << "\n--- Build Log Entry (ID: " << buildId << ") ---" << std::endl;
        log << "Date: " << std::put_time(std::localtime(&in_time_t), "%Y-%m-%d %X") << std::endl;
        log << "Build target: " << target << std::endl;
        log << "Output file: " << buildSystem.getConfig().getOutputFile() << std::endl;
        log << "Build type: " << (buildSystem.getConfig().getBuildType() == OreoBuild::BuildType::Debug ? "Debug" : "Release") << std::endl;
        log << "Total time: " << duration.count() << " µs (" 
            << std::fixed << std::setprecision(3) << duration.count() / 1000000.0 << " seconds)" << std::endl;
        log << "Files compiled: " << buildSystem.getFilesCompiled() << std::endl;
        log << "Up-to-date files: " << (buildSystem.getConfig().getSourceFiles().size() - buildSystem.getFilesCompiled()) << std::endl;
        log << "Build summary: " << buildSummary << std::endl;

        if (verbosityLevel >= OreoBuild::BuildSystem::VerbosityLevel::VeryVerbose) {
            log << "\nDetailed Build Information:" << std::endl;
            log << "Source files:" << std::endl;
            for (const auto& source : buildSystem.getConfig().getSourceFiles()) {
                log << "  " << source << std::endl;
            }
            log << "Include paths:" << std::endl;
            for (const auto& path : buildSystem.getConfig().getIncludePaths()) {
                log << "  " << path << std::endl;
            }
            log << "Libraries:" << std::endl;
            for (const auto& lib : buildSystem.getConfig().getLibraries()) {
                log << "  " << lib << std::endl;
            }
        }

        log.close();
        std::cout << Color::Green << "Build log (ID: " << buildId << ") appended to " << logFile << Color::Reset << std::endl;
    } else {
        std::cerr << Color::Red << "Failed to open build log file: " << logFile << Color::Reset << std::endl;
    }
}

std::string CLIHandler::generateBuildId() {
    auto now = std::chrono::system_clock::now();
    auto in_time_t = std::chrono::system_clock::to_time_t(now);
    std::stringstream ss;
    ss << std::put_time(std::localtime(&in_time_t), "%Y%m%d_%H%M%S");
    return ss.str();
}
