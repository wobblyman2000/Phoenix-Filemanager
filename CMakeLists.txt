cmake_minimum_required(VERSION 3.16)

# Project Name
project(arch_commander VERSION 1.13.0 LANGUAGES CXX)

# Force C++17 Standard compliance for modern Qt6 capabilities
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Enable automatic Meta-Object Compiler (MOC) handling for Qt signals/slots
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC OFF)
set(CMAKE_AUTORCC OFF)

# Locate Required Qt6 Libraries and Modules
find_package(Qt6 COMPONENTS 
    Widgets 
    Multimedia 
    MultimediaWidgets 
    REQUIRED
)

# Define the Executable Build Target using your main source file
add_executable(arch_commander main.cpp)

# Link your executable with the required Qt6 modules
target_link_libraries(arch_commander PRIVATE 
    Qt6::Widgets 
    Qt6::Multimedia 
    Qt6::MultimediaWidgets
)

# Optimization parameters for smooth desktop layout rendering performance
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()