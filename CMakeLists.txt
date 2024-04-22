cmake_minimum_required(VERSION 3.27.4)

project(heaphawk)

SET(SOURCE_FILES
    src/common.h
    src/common.cpp
    src/entry.h
    src/entry.cpp
    src/history.h
    src/history.cpp
    src/process.h
    src/process.cpp
    src/recorder.h
    src/recorder.cpp
    src/recorder.h
    src/snapshot.h
    src/snapshot.cpp
    src/main.cpp
    )

add_executable(heaphawk
               ${SOURCE_FILES}
               )

target_compile_features(heaphawk PRIVATE cxx_std_17)

if(MSVC)
    target_compile_options(heaphawk PRIVATE /W4 /WX)
else()
    target_compile_options(heaphawk PRIVATE
        -Wall
        -Wextra
        -Wpedantic
        )
endif()

set_property(TARGET heaphawk PROPERTY COMPILE_WARNING_AS_ERROR ON)
