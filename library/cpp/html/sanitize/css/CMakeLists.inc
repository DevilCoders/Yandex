SET(VER_MAJOR 1)
SET(VER_MINOR 4)
SET(VER_PATH  2)

IF(CMAKE_BUILD_TYPE == Debug)
    SET_APPEND(LEX_FLAGS -d )
ENDIF ()

PEERDIR(
    library/cpp/regex/pcre
)

CFLAGS(
    -DVER_FULL=${VER_FULL}
    -DVER_MAJOR=${VER_MAJOR}
    -DVER_MINOR=${VER_MINOR}
    -DVER_PATH=${VER_PATH}
)

SRCS(
    config_l.l
    css2_l.l
    config_y.y
    css2_y.y
    compile_regex.cpp
    config-drv.cpp
    css2-drv.cpp
    expressions.cpp
    sanit_config.cpp
    sanit_policy.cpp
    class_sanitize.cpp
    str_tools.cpp
    css_sanitize.cpp
)
