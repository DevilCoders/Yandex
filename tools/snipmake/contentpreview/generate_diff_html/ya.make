PROGRAM()

OWNER(yurikiselev)

ARCHIVE(
    NAME data.inc
    diff_first_part.rawhtml
    diff_second_part.rawhtml
)

PEERDIR(
    library/cpp/archive
    library/cpp/deprecated/prog_options
    yweb/robot/kiwi/clientlib
    yweb/robot/kiwi/protos
)

SRCS(
    main.cpp
)

END()
