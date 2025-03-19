LIBRARY()

OWNER(
    g:facts
)

PEERDIR(
    library/cpp/scheme
    library/cpp/string_utils/base64
)

SRCS(
    dynamic_list_replacer.cpp
)

END()

RECURSE_FOR_TESTS(
    ut
)
