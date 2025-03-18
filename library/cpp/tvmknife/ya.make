LIBRARY()

OWNER(g:passport_infra)

PEERDIR(
    library/cpp/colorizer
    library/cpp/tvmauth/client
    library/cpp/json
    library/cpp/ssh
    library/cpp/ssh_sign
    library/cpp/string_utils/base64
    library/cpp/string_utils/quote
    library/cpp/tvmknife/internal
)

SRCS(
    cache.cpp
    output.cpp
    simple_tvm_client.cpp
)

END()

RECURSE_FOR_TESTS(ut)
