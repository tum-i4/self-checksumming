cmake_minimum_required(VERSION 3.12)

project(self-checksumming VERSION 0.1 LANGUAGES CXX)

add_library(SCPass MODULE
        include/self-checksumming/DAGCheckersNetwork.h
        include/self-checksumming/CheckersNetworkBase.h
        include/self-checksumming/Stats.h

        src/DAGCheckersNetwork.cpp
        src/Stats.cpp
        src/SC.cpp
        )

add_library(self-checksumming::SCPass ALIAS SCPass)

add_library(Utils MODULE
        include/self-checksumming/FunctionInfo.h
        include/self-checksumming/FunctionMarker.h
        include/self-checksumming/FunctionFilter.h

        src/FunctionInfo.cpp
        src/FunctionMarker.cpp
        src/FunctionFilter.cpp
        )

add_library(self-checksumming::Utils ALIAS Utils)

add_library(SCPatchPass MODULE
        include/self-checksumming/PatchManifest.h

        src/PatchManifest.cpp
        src/SCPatch.cpp
        )

add_library(self-checksumming::SCPatchPass ALIAS SCPatchPass)

find_package(LLVM 6.0 REQUIRED CONFIG)
find_package(input-dependency REQUIRED COMPONENTS InputDependency)
find_package(nlohmann_json REQUIRED)

target_include_directories(SCPass
        PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
        PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/src
        ${LLVM_INCLUDE_DIRS})

target_include_directories(Utils
        PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
        PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/src
        ${LLVM_INCLUDE_DIRS})

target_include_directories(SCPatchPass
        PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
        PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/src
        ${LLVM_INCLUDE_DIRS})

if ($ENV{CLION_IDE})
    include_directories("/usr/include/llvm-6.0/")
    include_directories("/usr/include/llvm-c-6.0/")
endif ()


target_link_libraries(SCPass PRIVATE nlohmann_json::nlohmann_json input-dependency::InputDependency)

target_compile_features(SCPass PRIVATE cxx_std_11 cxx_range_for cxx_auto_type)
target_compile_features(Utils PRIVATE cxx_std_11 cxx_range_for cxx_auto_type)
target_compile_features(SCPatchPass PRIVATE cxx_std_11 cxx_range_for cxx_auto_type)

target_compile_options(SCPass PRIVATE -fno-rtti)
target_compile_options(Utils PRIVATE -fno-rtti)
target_compile_options(SCPatchPass PRIVATE -fno-rtti)


# Get proper shared-library behavior (where symbols are not necessarily
# resolved when the shared library is linked) on OS X.
#if(APPLE)
#    set_target_properties(SCPass PROPERTIES
#        LINK_FLAGS "-undefined dynamic_lookup"
#    )
#    set_target_properties(SCPatchPass PROPERTIES
#        LINK_FLAGS "-undefined dynamic_lookup"
#    )

#endif(APPLE)