#pragma once
#include <string>
#include <filesystem>

namespace OreoBuild {

class FileUtils {
public:
    static std::filesystem::file_time_type getLastModifiedTime(const std::string& filename);
    static bool isNewer(const std::string& file1, const std::string& file2);
    static void updateTimestamp(const std::string& filename);
    static void printFileInfo(const std::string& filename);  // Add this line
};

} // namespace OreoBuild
