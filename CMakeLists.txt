cmake_minimum_required(VERSION 3.19)
project(SmartSchedule LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

find_package(Qt6 6.5 REQUIRED COMPONENTS Core Gui Widgets Sql PrintSupport)

qt_standard_project_setup()

if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/QXlsx/QXlsx/CMakeLists.txt)
    add_subdirectory(QXlsx/QXlsx)
else()
    include(FetchContent)
    FetchContent_Declare(
        QXlsx
        GIT_REPOSITORY https://github.com/QtExcel/QXlsx.git
        GIT_TAG        v1.5.1
        SOURCE_SUBDIR  QXlsx
    )
    FetchContent_MakeAvailable(QXlsx)
endif()

qt_add_executable(SmartSchedule
    WIN32 MACOSX_BUNDLE
    main.cpp
    mainwindow.h mainwindow.cpp
    databasemanager.h databasemanager.cpp
    addentrydialog.h addentrydialog.cpp
    bellsdialog.h bellsdialog.cpp
    referencedialog.h referencedialog.cpp
    csvutils.h
    tablefile.h
    scheduleanalysis.h
    workloaddialog.h workloaddialog.cpp
    theme.h theme.cpp
)

target_link_libraries(SmartSchedule
    PRIVATE
        Qt6::Core
        Qt6::Gui
        Qt6::Widgets
        Qt6::Sql
        Qt6::PrintSupport
        QXlsx::QXlsx
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