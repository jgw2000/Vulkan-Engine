file(GLOB_RECURSE SRC_FILES Source/*.c??)
file(GLOB_RECURSE HEADER_FILES Source/*.h)

add_library(Engine ${SRC_FILES} ${HEADER_FILES})

target_compile_definitions(Engine PRIVATE BACKEND_SUPPORTS_VULKAN)
target_include_directories(Engine PUBLIC Source)

source_group(TREE ${PROJECT_SOURCE_DIR}/Engine FILES ${SRC_FILES} ${HEADER_FILES})