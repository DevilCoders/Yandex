LIBRARY()

OWNER(
    alexbykov
    mvel
    velavokr
)

SRCS(
    qd_factors_parser.cpp
    qd_indexer.cpp
    qd_limits.cpp
    qd_parser_utils.cpp
    qd_record_type.cpp
)

PEERDIR(
    library/cpp/string_utils/base64
    kernel/querydata/common
    kernel/querydata/idl
    library/cpp/binsaver
)

GENERATE_ENUM_SERIALIZATION(qd_record_type.h)

END()
