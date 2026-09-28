cmake_minimum_required(VERSION 3.21)
project(StudyBoard VERSION 0.3.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

find_package(Qt6 6.5 REQUIRED COMPONENTS Widgets Gui Core)

qt_add_executable(StudyBoard
    src/main.cpp
    src/MainWindow.h
    src/MainWindow.cpp
    src/BoardScene.h
    src/BoardScene.cpp
    src/BoardView.h
    src/BoardView.cpp
    src/BoardSerializer.h
    src/BoardSerializer.cpp
    resources/resources.qrc
)

target_link_libraries(StudyBoard PRIVATE Qt6::Widgets Qt6::Gui Qt6::Core)

if(MSVC)
    target_compile_options(StudyBoard PRIVATE /W4 /permissive-)
else()
    target_compile_options(StudyBoard PRIVATE -Wall -Wextra -Wpedantic)
endif()

if(WIN32)
    set_target_properties(StudyBoard PROPERTIES WIN32_EXECUTABLE TRUE)
endif()

install(TARGETS StudyBoard
    RUNTIME DESTINATION bin
    BUNDLE DESTINATION .
)

if(UNIX AND NOT APPLE)
    install(FILES resources/io.github.studyboard.StudyBoard.desktop
        DESTINATION share/applications)
    install(FILES resources/io.github.studyboard.StudyBoard.svg
        DESTINATION share/icons/hicolor/scalable/apps)
endif()
