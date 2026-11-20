
cmake_minimum_required(VERSION 3.16)

project(StudentManagementSystem VERSION 0.1 LANGUAGES CXX)

set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(QT NAMES Qt6 Qt5 REQUIRED COMPONENTS Widgets Sql)
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Widgets Sql)

set(PROJECT_SOURCES
    main.cpp
    authmanager.cpp
    authmanager.h
    logindialog.cpp
    logindialog.h
    studentportalwindow.cpp
    studentportalwindow.h
    mainwindow.cpp
    mainwindow.h
    mainwindow.ui
    databasemanager.cpp
    databasemanager.h
    student.cpp
    student.h
    course.cpp
    course.h
    studentcourseresult.cpp
    studentcourseresult.h
    gradetype.cpp
    gradetype.h
    studentrepository.cpp
    studentrepository.h
    courserepository.cpp
    courserepository.h
    studentcourseresultrepository.cpp
    studentcourseresultrepository.h
    dataimporter.cpp
    dataimporter.h
)

if(${QT_VERSION_MAJOR} GREATER_EQUAL 6)
    qt_add_executable(StudentManagementSystem
        MANUAL_FINALIZATION
        ${PROJECT_SOURCES}
        databasemanager.h databasemanager.cpp
        student.h student.cpp
        course.h course.cpp
        studentcourseresult.h studentcourseresult.cpp
        gradetype.h gradetype.cpp
        studentrepository.h studentrepository.cpp
        courserepository.h courserepository.cpp
        studentcourseresultrepository.h studentcourseresultrepository.cpp
        resources.qrc
        resources.qrc

    )
else()
    if(ANDROID)
        add_library(StudentManagementSystem SHARED
            ${PROJECT_SOURCES}
        )
    else()
        add_executable(StudentManagementSystem
            ${PROJECT_SOURCES}
        )
    endif()
endif()

target_link_libraries(StudentManagementSystem PRIVATE
    Qt${QT_VERSION_MAJOR}::Widgets
    Qt${QT_VERSION_MAJOR}::Sql
)

if(${QT_VERSION} VERSION_LESS 6.1.0)
    set(BUNDLE_ID_OPTION MACOSX_BUNDLE_GUI_IDENTIFIER com.example.StudentManagementSystem)
endif()

set_target_properties(StudentManagementSystem PROPERTIES
    ${BUNDLE_ID_OPTION}
    MACOSX_BUNDLE_BUNDLE_VERSION ${PROJECT_VERSION}
    MACOSX_BUNDLE_SHORT_VERSION_STRING ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}
    MACOSX_BUNDLE TRUE
    WIN32_EXECUTABLE TRUE
)

include(GNUInstallDirs)

install(TARGETS StudentManagementSystem
    BUNDLE DESTINATION .
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

if(QT_VERSION_MAJOR EQUAL 6)
    qt_finalize_executable(StudentManagementSystem)
endif()
