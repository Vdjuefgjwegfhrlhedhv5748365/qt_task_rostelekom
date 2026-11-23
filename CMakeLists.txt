cmake_minimum_required(VERSION 3.16)
project(ShapeEditor LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)

find_package(Qt6 REQUIRED COMPONENTS Widgets Sql)

add_executable(ShapeEditor MACOSX_BUNDLE
    main.cpp
    src/shapedatabase.cpp
    src/shapescene.cpp
    src/ui/mainwindow.cpp
    src/ui/shapetable.cpp
    "Header Files/shapedatabase.h"
    "Header Files/shapescene.h"
    "Header Files/ui/mainwindow.h"
    "Header Files/ui/shapetable.h"
    forms/mainwindow.ui
)
set_property(TARGET ShapeEditor PROPERTY AUTOUIC_SEARCH_PATHS
    "${CMAKE_CURRENT_SOURCE_DIR}/forms"
)
target_include_directories(ShapeEditor PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/Header Files")
target_link_libraries(ShapeEditor PRIVATE Qt6::Widgets Qt6::Sql)

