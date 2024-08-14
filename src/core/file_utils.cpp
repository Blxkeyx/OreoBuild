#include "file_utils.hpp"
#include <iostream>
#include <iomanip>
#include <ctime>

namespace OreoBuild {

std::filesystem::file_time_type FileUtils::getLastModifiedTime(const std::string& filename) {
    try {
        return std::filesystem::last_write_time(filename);
    } catch (const std::filesystem::filesystem_error& e) {
        std::cerr << "Warning: Unable to get last modified time for " << filename << ": " << e.what() << std::endl;
        return std::filesystem::file_time_type::min();
    }
}

bool FileUtils::isNewer(const std::string& file1, const std::string& file2) {
    if (!std::filesystem::exists(file2)) {
        std::cout << file2 << " does not exist. Considering " << file1 << " as newer." << std::endl;
        return true;
    }
    if (!std::filesystem::exists(file1)) {
        std::cout << file1 << " does not exist. It's not newer than " << file2 << "." << std::endl;
        return false;
    }
    auto time1 = getLastModifiedTime(file1);
    auto time2 = getLastModifiedTime(file2);
    bool newer = time1 > time2;
    std::cout << file1 << " is " << (newer ? "newer" : "older") << " than " << file2 << std::endl;
    return newer;
}

void FileUtils::updateTimestamp(const std::string& filename) {
    try {
        std::filesystem::last_write_time(filename, std::filesystem::file_time_type::clock::now());
    } catch (const std::filesystem::filesystem_error& e) {
        std::cerr << "Warning: Unable to update timestamp for " << filename << ": " << e.what() << std::endl;
    }
}

void FileUtils::printFileInfo(const std::string& filename) {
    std::cout << "File info for " << filename << ":" << std::endl;
    if (std::filesystem::exists(filename)) {
        auto lastModTime = getLastModifiedTime(filename);
        auto sctp = std::chrono::time_point_cast<std::chrono::system_clock::duration>(lastModTime - std::filesystem::file_time_type::clock::now() + std::chrono::system_clock::now());
        std::time_t cftime = std::chrono::system_clock::to_time_t(sctp);
        std::cout << "  Last modified: " << std::put_time(std::localtime(&cftime), "%F %T") << std::endl;
        std::cout << "  Size: " << std::filesystem::file_size(filename) << " bytes" << std::endl;
    } else {
        std::cout << "  File does not exist" << std::endl;
    }
}

}
