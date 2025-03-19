cmake_minimum_required(VERSION 2.6)

# Please do not delete this. This is an de-arcadificated matrixnet calcer.
macro(OWNER)
endmacro(OWNER)
OWNER(
    g:base
    lagrunge
    mvel
    ironpeter
    vitbar
    kirillovs
)

# The matrixnet library project.
project(matrixnet_library)

# matrixnet_library
set (MATRIXNET_LIBRARY_SOURCES
    ../model_info.h
    ../relev_calcer.h
    ../mn_sse.h
    ../mn_sse.cpp
    ../mn_file.h
    ../mn_file.cpp
    matrixnet.fbs.h
    flatbuffers/flatbuffers.h
    util/compiler.h
    util/filemap.h
    util/filemap.cpp
    util/map.h
    util/ptr.h
    util/set.h
    util/string_cast.h
    util/string_cast.cpp
    util/stroka.h
    util/types.h
    util/utility.h
    util/vector.h
    util/yexception.h
    util/yexception.cpp
    util/ylimits.h
)

add_library(matrixnet_library STATIC ${MATRIXNET_LIBRARY_SOURCES})

option (NO_EXCEPTIONS "Disable throwing C++ exceptions" OFF)
add_definitions(-std=c++11)

target_compile_definitions(matrixnet_library
    PUBLIC MATRIXNET_WITHOUT_ARCADIA
)

if(NO_EXCEPTIONS)
    target_compile_definitions(matrixnet_library
        PUBLIC MATRIXNET_WITHOUT_ARCADIA_NO_EXCEPTIONS
    )
endif()

# matrixnet_demo
set (MATRIXNET_DEMO_SOURCES
    demo/demo.cpp
)

add_executable(matrixnet_demo ${MATRIXNET_DEMO_SOURCES})

target_link_libraries(matrixnet_demo matrixnet_library)

add_custom_command(TARGET matrixnet_demo POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E echo "${CMAKE_CURRENT_SOURCE_DIR}/demo/data"
    > "$<TARGET_FILE_DIR:matrixnet_demo>/path_to_data_dir.txt"
)
