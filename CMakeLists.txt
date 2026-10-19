cmake_minimum_required(VERSION 3.19)
project(SmartSchedule LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

find_package(Qt6 6.5 REQUIRED COMPONENTS Core Widgets Sql PrintSupport)

qt_standard_project_setup()

qt_add_executable(SmartSchedule
    WIN32 MACOSX_BUNDLE
    main.cpp
    mainwindow.cpp
    mainwindow.h
    mainwindow.ui
    databasemanager.h databasemanager.cpp
    addentrydialog.h addentrydialog.cpp
    bellsdialog.h bellsdialog.cpp
)

target_link_libraries(SmartSchedule
    PRIVATE
        Qt::Core
        Qt::Widgets
        Qt6::Sql
        Qt6::PrintSupport
)

install(TARGETS SmartSchedule
    BUNDLE  DESTINATION .
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
)

qt_generate_deploy_app_script(
    TARGET SmartSchedule
    OUTPUT_SCRIPT deploy_script
    NO_UNSUPPORTED_PLATFORM_ERROR
)
install(SCRIPT ${deploy_script})
