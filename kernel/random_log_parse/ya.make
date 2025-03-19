LIBRARY()

OWNER(mvel)

PEERDIR(
    kernel/factor_slices
    kernel/factor_storage
    kernel/factors_util
    library/cpp/json
    library/cpp/json/writer
    library/cpp/string_utils/base64
)

SRCS(
    random_log_parse.cpp
)

END()
