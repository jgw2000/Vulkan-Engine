file(GLOB_RECURSE SRC_FILES *.c??)
file(GLOB_RECURSE HEADER_FILES *.h)

add_executable(TriangleSample ${SRC_FILES} ${HEADER_FILES})
target_link_libraries(TriangleSample PRIVATE VE)

set_target_properties(TriangleSample PROPERTIES VS_DEBUGGER_WORKING_DIRECTORY "${PROJECT_SOURCE_DIR}")

set_property(TARGET TriangleSample PROPERTY FOLDER "Samples")