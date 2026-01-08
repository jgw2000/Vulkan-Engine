cmake_minimum_required(VERSION 4.2.1)

project(VulkanEngine VERSION 1.0)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED True)
set(CMAKE_CXX_SCAN_FOR_MODULES ON)

include($ENV{VCPKG_ROOT}/scripts/buildsystems/vcpkg.cmake)
find_package(Vulkan REQUIRED)
find_package(glfw3 CONFIG REQUIRED)
find_package(glm CONFIG REQUIRED)

#target_compile_definitions(VulkanCppModule PUBLIC VULKAN_HPP_DISPATCH_LOADER_DYNAMIC=1 VULKAN_HPP_NO_STRUCT_CONSTRUCTORS=1)
#target_include_directories(VulkanCppModule PUBLIC "${Vulkan_INCLUDE_DIR}")

# Where our engine code is
set(ENGINE ${CMAKE_CURRENT_SOURCE_DIR}/Engine)
add_subdirectory(${ENGINE})

set(SAMPLE ${CMAKE_CURRENT_SOURCE_DIR}/Samples)
add_subdirectory(${SAMPLE}/Triangle)

# ==================================================================================================
# Sub-projects
# ==================================================================================================
#find_package(Vulkan REQUIRED volk SPIRV-Tools)
include_directories(${Vulkan_INCLUDE_DIR})

#set(SDL_STATIC ON CACHE BOOL "" FORCE)
#add_subdirectory(${EXTERNAL}/sdl)
#add_subdirectory(${EXTERNAL}/glm)
#add_subdirectory(${ENGINE})

#target_link_libraries(Engine PUBLIC glm)
#target_link_libraries(Engine PUBLIC SDL3::SDL3-static)

#set_property(TARGET glm PROPERTY FOLDER "ThirdParty")
#set_property(TARGET SDL_uclibc PROPERTY FOLDER "ThirdParty")
#set_property(TARGET SDL3_test PROPERTY FOLDER "ThirdParty")
#set_property(TARGET SDL3-shared PROPERTY FOLDER "ThirdParty")
#set_property(TARGET SDL3-static PROPERTY FOLDER "ThirdParty")