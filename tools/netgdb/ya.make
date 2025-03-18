PROGRAM()

OWNER(druxa)

PEERDIR(
    library/cpp/deprecated/prog_options
    library/cpp/netliba/v12
    library/cpp/regex/pcre
    quality/deprecated/Misc
)

SRCS(
    client.cpp
    requests.cpp
    fire.cpp
    daemon.cpp
    shell.cpp
    distribute.cpp
)

NO_BUILD_IF(OS_WINDOWS)

SET(USE_LF_ALLOCATOR yes)

END()
