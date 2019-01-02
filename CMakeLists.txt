cmake_minimum_required(VERSION 3.0)

project(SSchedSimulator)

set(CMAKE_CXX_STANDARD 11)

# Enable test
enable_testing()

option(WITH_TESTS "Build with tests enabled" ON)
option(BUILD_SEPARATE_MODELS "Build each module as a separate library" OFF)
option(BUILD_SHARED_LIBS "Build as shared libraries" OFF)
option(USE_PREDOWNDLOAD_DEPS "Build with pre-downloaded dependencies" OFF)
option(ENABLE_HDRHISTOGRAM "Build with HdrHistogram library enabled" ON)


# libraries
set(saber_libraries "")


# Whether or not to use pre-downloaded packages
if(USE_PREDOWNLOAD_DEPS)
    message("You choose to use pre-downaloaded dependencies")
    include_directories("${CMAKE_CURRENT_SOURCE_DIR}/include/third-party")
    # add libraries here
else()
    message("You choose to let cmake to download & build the dependencies on the fly")
    # Download dependencies
    # # Download json c++
    configure_file("cmake/CMakeLists.txt.json.in" "${CMAKE_CURRENT_BINARY_DIR}/json-download/CMakeLists.txt")
    execute_process(COMMAND ${CMAKE_COMMAND} -G "${CMAKE_GENERATOR}" .
         RESULT_VARIABLE result
         WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/json-download )

    if(result)
        message(FATAL_ERROR "CMake step for json failed: ${result}")
    endif()

    add_subdirectory("${CMAKE_CURRENT_BINARY_DIR}/json-download"
        EXCLUDE_FROM_ALL)
    include_directories("${CMAKE_CURRENT_BINARY_DIR}/include/third-party")

    # # Download catch2
    configure_file("cmake/CMakeLists.txt.catch2.in" "${CMAKE_CURRENT_BINARY_DIR}/catch2-download/CMakeLists.txt")
    execute_process(COMMAND ${CMAKE_COMMAND} -G "${CMAKE_GENERATOR}" .
        RESULT_VARIABLE result
        WORKING_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/catch2-download" )

    if(result)
        message(FATAL_ERROR "CMake step for catch2 failed: ${result}")
    endif()

    add_subdirectory("${CMAKE_CURRENT_BINARY_DIR}/catch2-download"
        EXECLUDE_FROM_ALL)
    include_directories("${CMAKE_CURRENT_BINARY_DIR}/include/third-party")

    ## Download & Compile Hdr_Histogram_C
    if(ENABLE_HDRHISTOGRAM)
        configure_file("cmake/CMakeLists.txt.hdrhistogram_c.in" ${CMAKE_BINARY_DIR}/HdrHistogram_c-download/CMakeLists.txt)
        execute_process(COMMAND ${CMAKE_COMMAND} -G "${CMAKE_GENERATOR}" .
                RESULT_VARIABLE result
                WORKING_DIRECTORY ${CMAKE_BINARY_DIR}/HdrHistogram_c-download)
        if (result)
            message(FATAL_ERROR "CMake step for HdrHistogram_cHdrHistogram_c failed: ${result}")
        endif ()
        execute_process(COMMAND ${CMAKE_COMMAND} --build .
                RESULT_VARIABLE result
                WORKING_DIRECTORY ${CMAKE_BINARY_DIR}/HdrHistogram_c-download)
        if (result)
            message(FATAL_ERROR "Build step for HdrHistogram_c failed: ${result}")
        endif ()

        add_subdirectory(${CMAKE_BINARY_DIR}/HdrHistogram_c-src
                ${CMAKE_BINARY_DIR}/HdrHistogram_c-build
                EXCLUDE_FROM_ALL)

        list(APPEND saber_libraries hdr_histogram_static)
    endif()

    ## Download & Compiler LEMON C++
    configure_file("cmake/CMakeLists.txt.lemon.in" ${CMAKE_BINARY_DIR}/lemon-download/CMakeLists.txt)
    execute_process(COMMAND ${CMAKE_COMMAND} -G "${CMAKE_GENERATOR}" .
            RESULT_VARIABLE result
            WORKING_DIRECTORY ${CMAKE_BINARY_DIR}/lemon-download)
    if (result)
        message(FATAL_ERROR "CMake step for lemon failed: ${result}")
    endif ()
    execute_process(COMMAND ${CMAKE_COMMAND} --build .
            RESULT_VARIABLE result
            WORKING_DIRECTORY ${CMAKE_BINARY_DIR}/lemon-download)
    if (result)
        message(FATAL_ERROR "Build step for lemon failed: ${result}")
    endif ()

    add_subdirectory(${CMAKE_BINARY_DIR}/lemon-src
            ${CMAKE_BINARY_DIR}/lemon-download
            EXCLUDE_FROM_ALL)
    list(APPEND saber_libraries emon)
endif()

if(ENABLE_HDRHISTOGRAM)
    add_definitions(-DENABLE_HDRHISTOGRAM)
endif()


# includes
include_directories("${CMAKE_CURRENT_SOURCE_DIR}/src")
include_directories("${CMAKE_CURRENT_SOURCE_DIR}/include/saber")


add_subdirectory(src)
add_subdirectory(test)
add_subdirectory(example)

