#pragma once

#include <string>
#include <cstdint>

// MD5 hash utility
namespace Md5
{
    // Compute MD5 hash of input string, returns 32-char hex digest
    std::string hash(const std::string& input);

    // Compute MD5 hash of raw bytes
    std::string hash(const uint8_t* data, size_t length);
}
