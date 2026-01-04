#pragma once

namespace VE::backend {
    
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

} // namespace VK::backend