PROGRAM()

OWNER(vdmit)

SRCS(
    json_to_jsonlines_main.cpp
)

PEERDIR(
    library/cpp/getopt
)

END()

RECURSE(ut)
