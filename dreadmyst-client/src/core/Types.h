#pragma once

#include <cstdint>
#include <string>
#include <memory>
#include <vector>
#include <map>
#include <functional>

// Standard type aliases matching the original binary
using u8  = uint8_t;
using u16 = uint16_t;
using u32 = uint32_t;
using u64 = uint64_t;
using s8  = int8_t;
using s16 = int16_t;
using s32 = int32_t;
using s64 = int64_t;
using f32 = float;
using f64 = double;

// GUID type used for all entity identification
using ObjectGuid = u32;
constexpr ObjectGuid INVALID_GUID = 0;
