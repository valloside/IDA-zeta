cmake_minimum_required(VERSION 3.10.0)
project(ida-zeta LANGUAGES C CXX)

# IDA SDK Configuration
set(IDA_SDK_DIR "" CACHE PATH "Path to IDA Pro SDK")
set(IDA_INSTALL_DIR "" CACHE PATH "Path to IDA Pro installation directory (e.g., C:/Program Files/IDA Pro 9.1)")
if(NOT IDA_INSTALL_DIR)
    message(FATAL_ERROR "IDA_SDK_DIR is not set. Please specify the IDA SDK directory.")
endif()

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include_directories(${IDA_SDK_DIR}/include)
link_directories(${IDA_SDK_DIR}/lib/x64_win_vc_64)

add_compile_options("/utf-8" "/W3")

# Compiler definitions for IDA plugin
add_definitions(-D__NT__ -D__EA64__)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

add_library(ida-zeta SHARED
    src/ida-zeta.cpp
    src/AssignType.cpp
    src/FixSetTypeCrash.cpp
    src/SetAllFuncsUserdefined.cpp
    src/EditFunctionPro.cpp
    src/FixFrameSizeCalc.cpp
    src/CreateSubObjectType.cpp
)
target_link_libraries(ida-zeta ida)

if(IDA_INSTALL_DIR)
    install(TARGETS ida-zeta RUNTIME DESTINATION "${IDA_INSTALL_DIR}/plugins" COMPONENT)
endif()
