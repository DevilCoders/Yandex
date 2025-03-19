LIBRARY()

OWNER(
    alexbykov
    mvel
    velavokr
)

SRCS(
    qi_parser.cpp
)

PEERDIR(
    library/cpp/string_utils/base64
    kernel/querydata/client
    kernel/querydata/idl
    kernel/querydata/indexer2
    library/cpp/binsaver
    library/cpp/json
)

END()
