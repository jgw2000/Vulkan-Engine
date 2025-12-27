file(GLOB_RECURSE SRC_FILES source/*.c??)
file(GLOB_RECURSE HEADER_FILES source/*.h)

add_library(Engine ${SRC_FILES} ${HEADER_FILES})

target_include_directories(Engine PUBLIC source)

source_group(TREE ${PROJECT_SOURCE_DIR}/engine FILES ${SRC_FILES} ${HEADER_FILES})