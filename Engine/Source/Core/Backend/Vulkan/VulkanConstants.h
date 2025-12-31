#pragma once

#include <cassert>
#include <vector>

#include <Platform/Types.h>

namespace VE::backend {

#define FVK_DEBUG_SYSTRACE                0x00000001

// Group markers are used to denote collections of GPU commands.  It is typically at the
// granualarity of a renderpass. You can enable this along with FVK_DEBUG_DEBUG_UTILS to take
// advantage of vkCmdBegin/EndDebugUtilsLabelEXT. You can also just enable this with
// FVK_DEBUG_PRINT_GROUP_MARKERS to print the current marker to stdout.
#define FVK_DEBUG_GROUP_MARKERS           0x00000002

#define FVK_DEBUG_TEXTURE                 0x00000004
#define FVK_DEBUG_LAYOUT_TRANSITION       0x00000008
#define FVK_DEBUG_COMMAND_BUFFER          0x00000010
#define FVK_DEBUG_DUMP_API                0x00000020
#define FVK_DEBUG_VALIDATION              0x00000040
#define FVK_DEBUG_PRINT_GROUP_MARKERS     0x00000080
#define FVK_DEBUG_BLIT_FORMAT             0x00000100
#define FVK_DEBUG_BLITTER                 0x00000200
#define FVK_DEBUG_FBO_CACHE               0x00000400
#define FVK_DEBUG_SHADER_MODULE           0x00000800
#define FVK_DEBUG_READ_PIXELS             0x00001000
#define FVK_DEBUG_PIPELINE_CACHE          0x00002000
#define FVK_DEBUG_STAGING_ALLOCATION      0x00004000

// Enable the debug utils extension if it is available.
#define FVK_DEBUG_DEBUG_UTILS             0x00008000

// Use this to debug potential Handle/Resource leakage. It will print out reference counts for all
// the currently active resources.
#define FVK_DEBUG_RESOURCE_LEAK           0x00010000

// Set this to enable logging "only" to one output stream. This is useful in the case where we want
// to debug with print statements and want ordered logging (e.g LOG(INFO) and LOG(ERROR) will not
// appear in order of calls).
#define FVK_DEBUG_FORCE_LOG_TO_I          0x00020000

// Enable a minimal set of traces to assess the performance of the backend.
// All other debug features must be disabled.
#define FVK_DEBUG_PROFILING               0x00040000

#define FVK_DEBUG_VULKAN_BUFFER_CACHE     0x00080000

// Useful default combinations
#define FVK_DEBUG_EVERYTHING              (0xFFFFFFFF & ~FVK_DEBUG_PROFILING)
#define FVK_DEBUG_PERFORMANCE     \
    FVK_DEBUG_SYSTRACE

#ifndef NDEBUG
#define FVK_DEBUG_FLAGS FVK_DEBUG_EVERYTHING
#else
#define FVK_DEBUG_FLAGS 0
#endif

#define FVK_ENABLED(flags) (((FVK_DEBUG_FLAGS) & (flags)) == (flags))

// All vkCreate* functions take an optional allocator. For now we select the default allocator by
// passing in a null pointer, and we highlight the argument by using the VKALLOC constant.
constexpr struct VkAllocationCallbacks* VKALLOC = nullptr;

constexpr static const int FVK_REQUIRED_VERSION_MAJOR = 1;
constexpr static const int FVK_REQUIRED_VERSION_MINOR = 4;

#define EXPAND_ENUM(...)                                                          \
    u32 size = 0;                                                                 \
    VkResult result = func(__VA_ARGS__, nullptr);                                 \
    assert(result == VK_SUCCESS && "enumerate size error");                       \
    std::vector<OutType> ret(size);                                               \
    result = func(__VA_ARGS__, ret.data());                                       \
    assert(result == VK_SUCCESS && "enumerate error");                            \
    return std::move(ret);

#define EXPAND_ENUM_NO_ARGS() EXPAND_ENUM(&size)
#define EXPAND_ENUM_ARGS(...) EXPAND_ENUM(__VA_ARGS__, &size)

template <typename OutType>
std::vector<OutType> VkEnumerate(VKAPI_ATTR VkResult(*func)(u32*, OutType*)) {
    EXPAND_ENUM_NO_ARGS();
}

template <typename InType, typename OutType>
std::vector<OutType> VkEnumerate(VKAPI_ATTR VkResult(*func)(InType, u32*, OutType*), InType inData) {
    EXPAND_ENUM_ARGS(inData);
}

template <typename InTypeA, typename InTypeB, typename OutType>
std::vector<OutType> VkEnumerate(VKAPI_ATTR VkResult(*func)(InTypeA, InTypeB, u32*, OutType*), InTypeA inDataA, InTypeB inDataB) {
    EXPAND_ENUM_ARGS(inDataA, inDataB);
}

#undef EXPAND_ENUM
#undef EXPAND_ENUM_NO_ARGS
#undef EXPAND_ENUM_ARGS

} // namespace VE::backend