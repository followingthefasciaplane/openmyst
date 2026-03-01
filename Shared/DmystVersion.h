#pragma once

#include <cstdint>

// Client revision: r1189 (0x4A5)
// Server revision: r1188 (0x4A4)
// NOTE: Using server revision for version check compatibility
constexpr uint16_t DMYST_REVISION      = 1188;
constexpr uint16_t DMYST_REVISION_HEX  = 0x4A4;

constexpr const char* DMYST_VERSION_STRING = "1.0.1189";
