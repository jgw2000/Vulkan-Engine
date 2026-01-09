file(GLOB_RECURSE SRC_FILES Src/*.c??)
file(GLOB_RECURSE HEADER_FILES Inc/*.h)

add_library(VE ${SRC_FILES} ${HEADER_FILES})

target_compile_definitions(VE PUBLIC VULKAN_HPP_DISPATCH_LOADER_DYNAMIC=1 VULKAN_HPP_NO_STRUCT_CONSTRUCTORS=1)
target_include_directories(VE PUBLIC Inc)
target_link_libraries(VE PUBLIC Vulkan::Vulkan)
target_link_libraries(VE PUBLIC glfw)

source_group(TREE ${PROJECT_SOURCE_DIR}/Engine FILES ${SRC_FILES} ${HEADER_FILES})