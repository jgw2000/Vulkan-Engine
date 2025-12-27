# ==================================================================================================
# CMake
# ==================================================================================================
cmake_minimum_required(VERSION 4.2.1)

# ==================================================================================================
# Project declaration
# ==================================================================================================
project(VulkanEngine VERSION 1.0)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED True)

# ==================================================================================================
#  Paths
# ==================================================================================================
# Where our external libs are
set(EXTERNAL ${CMAKE_CURRENT_SOURCE_DIR}/ThirdParty)

# Where our engine code is
set(ENGINE ${CMAKE_CURRENT_SOURCE_DIR}/Engine)

set(SAMPLE ${CMAKE_CURRENT_SOURCE_DIR}/Samples)

# ==================================================================================================
# OS specific
# ==================================================================================================
if (NOT ANDROID AND NOT WEBGL AND NOT IOS)
    set(IS_HOST_PLATFORM TRUE)
endif()

# ==================================================================================================
# Sub-projects
# ==================================================================================================
add_subdirectory(${EXTERNAL}/glm)
add_subdirectory(${ENGINE})

target_include_directories(Engine PUBLIC ${EXTERNAL})
target_link_libraries(Engine PUBLIC glm-header-only)

if (IS_HOST_PLATFORM)
endif()

# ==================================================================================================
# Samples
# ==================================================================================================
add_subdirectory(${SAMPLE}/Test)