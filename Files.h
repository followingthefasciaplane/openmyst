#pragma once

#include <string>
#include <vector>

namespace Files
{
    // Check if a file exists
    bool exists(const std::string& path);

    // Read entire file to string
    std::string readAll(const std::string& path);

    // Write string to file
    bool writeAll(const std::string& path, const std::string& content);

    // List files in a directory
    std::vector<std::string> listFiles(const std::string& directory, const std::string& extension = "");

    // Get file extension (lowercase)
    std::string getExtension(const std::string& path);

    // Get filename without path
    std::string getFilename(const std::string& path);

    // Get directory from full path
    std::string getDirectory(const std::string& path);

    // Ensure directory exists (create if needed)
    bool ensureDirectory(const std::string& path);
}
