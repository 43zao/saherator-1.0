cmake_minimum_required(VERSION 3.10)

project(Saherator)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include_directories(include)

file(GLOB SOURCES
    src/*.cpp
)

add_executable(Saherator ${SOURCES})

# Default build type
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Debug)
endif()

if(MSVC)

    target_compile_options(Saherator PRIVATE
        /W4
    )

    # Release optimizations
    target_compile_options(Saherator PRIVATE
        $<$<CONFIG:Release>:/O2>
    )

else()

    # Common flags
    target_compile_options(Saherator PRIVATE
        -Wall
        -Wextra
    )

    # Default optimization level
    target_compile_options(Saherator PRIVATE
        $<$<CONFIG:Debug>:-O2>
    )

    # Strong release build
    target_compile_options(Saherator PRIVATE
        $<$<CONFIG:Release>:
            -O3
            -march=native
            -flto
        >
    )

    target_link_options(Saherator PRIVATE
        $<$<CONFIG:Release>:-flto>
    )

endif()