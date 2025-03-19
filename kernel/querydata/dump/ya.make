LIBRARY()

OWNER(
    alexbykov
    mvel
    velavokr
)

SRCS(
    qd_dump.cpp
)

PEERDIR(
    kernel/querydata/client
    kernel/querydata/idl
    library/cpp/json
    library/cpp/scheme
    library/cpp/string_utils/relaxed_escaper
)

END()
