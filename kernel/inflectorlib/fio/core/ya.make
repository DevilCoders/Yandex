LIBRARY()

OWNER(
    gotmanov
)

SRCS(
    fio_exceptions.cpp
    fio_inflector_core.cpp
    fio_query.cpp
    fio_text.cpp
    fio_token.cpp
)

PEERDIR(
    kernel/lemmer/alpha
    kernel/lemmer/core
    kernel/lemmer/dictlib
    library/cpp/string_utils/scan
    library/cpp/tokenizer
)

END()
