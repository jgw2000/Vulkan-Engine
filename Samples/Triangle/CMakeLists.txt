file(GLOB_RECURSE SRC_FILES *.c??)
file(GLOB_RECURSE HEADER_FILES *.h)

add_executable(TriangleSample ${SRC_FILES} ${HEADER_FILES})

if(ENABLE_CPP20_MODULE)
    target_compile_definitions(TriangleSample PRIVATE USE_CPP20_MODULES=1)
endif()

target_link_libraries(TriangleSample PRIVATE VE)

set_property(TARGET TriangleSample PROPERTY FOLDER "Samples")