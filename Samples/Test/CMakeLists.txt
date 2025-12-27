file(GLOB_RECURSE SRC_FILES *.c??)
file(GLOB_RECURSE HEADER_FILES *.h)

add_executable(Test ${SRC_FILES} ${HEADER_FILES})
set_property(TARGET Test PROPERTY FOLDER "Samples")
target_link_libraries(Test PUBLIC Engine)

source_group(TREE ${PROJECT_SOURCE_DIR}/Samples/Test FILES ${SRC_FILES} ${HEADER_FILES})
