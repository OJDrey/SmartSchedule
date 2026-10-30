cmake_minimum_required(VERSION 3.19)
project(SmartSchedule VERSION 1.5.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

set(QT_NO_PRIVATE_MODULE_WARNING ON)
find_package(Qt6 6.5 REQUIRED COMPONENTS Core Gui Widgets Sql PrintSupport)
if(Qt6Gui_VERSION VERSION_GREATER_EQUAL "6.10.0")
    find_package(Qt6 REQUIRED COMPONENTS GuiPrivate)
endif()

qt_standard_project_setup()

if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/QXlsx/QXlsx/CMakeLists.txt)
    set(QXLSX_DIR ${CMAKE_CURRENT_SOURCE_DIR}/QXlsx/QXlsx)
else()
    include(FetchContent)
    FetchContent_Declare(
        QXlsx
        GIT_REPOSITORY https://github.com/QtExcel/QXlsx.git
        GIT_TAG        v1.5.1
        SOURCE_SUBDIR  no_cmake_subdir
    )
    FetchContent_MakeAvailable(QXlsx)
    set(QXLSX_DIR ${qxlsx_SOURCE_DIR}/QXlsx)
endif()

file(GLOB QXLSX_SOURCES CONFIGURE_DEPENDS ${QXLSX_DIR}/source/*.cpp)
file(GLOB QXLSX_HEADERS CONFIGURE_DEPENDS ${QXLSX_DIR}/header/*.h)
set_source_files_properties(${QXLSX_SOURCES} PROPERTIES COMPILE_DEFINITIONS
    "QT_NO_KEYWORDS;QT_NO_CAST_TO_ASCII;QT_NO_CAST_FROM_ASCII;QT_NO_URL_CAST_FROM_STRING;QT_NO_CAST_FROM_BYTEARRAY;QT_USE_QSTRINGBUILDER;QT_NO_SIGNALS_SLOTS_KEYWORDS;QT_USE_FAST_OPERATOR_PLUS")

configure_file(version.h.in ${CMAKE_CURRENT_BINARY_DIR}/version.h @ONLY)

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
    theme.h theme.cpp
    scheduleanalysis.h
    schedulefilter.h
    schedulegrid.h schedulegrid.cpp
    filterbar.h filterbar.cpp
    schedulehtml.h schedulehtml.cpp
    scheduleimporter.h scheduleimporter.cpp
    importscheduledialog.h importscheduledialog.cpp
    conflictreportdialog.h conflictreportdialog.cpp
    entityscheduledialog.h entityscheduledialog.cpp
    workloaddialog.h workloaddialog.cpp
    roomloaddialog.h roomloaddialog.cpp
    ${QXLSX_SOURCES}
    ${QXLSX_HEADERS}
)

if(WIN32 AND EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/app.ico)
    target_sources(SmartSchedule PRIVATE app.rc)
endif()

target_include_directories(SmartSchedule PRIVATE ${CMAKE_CURRENT_BINARY_DIR} ${QXLSX_DIR}/header)

target_link_libraries(SmartSchedule
    PRIVATE
        Qt6::Core
        Qt6::Gui
        Qt6::Widgets
        Qt6::Sql
        Qt6::PrintSupport
        Qt6::GuiPrivate
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

include(cmake/Packaging.cmake)