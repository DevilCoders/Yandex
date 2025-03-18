OWNER(
    petrk
    yurakura
)

LIBRARY()

SRCS(
    main.cpp
    mode_build.cpp
    mode_build_file.cpp
    mode_build_remote.cpp
    mode_print.cpp
    mode_validate.cpp
    parse_json_export.cpp
    validate.cpp
)

PEERDIR(
    kernel/geodb
    kernel/search_types
    library/cpp/binsaver
    library/cpp/getopt/small
    library/cpp/json
    library/cpp/langs
    library/cpp/logger/global
    library/cpp/neh
    library/cpp/streams/factory
    library/cpp/timezone_conversion
    util/draft
)

END()
