LIBRARY()

OWNER(
    g:app_host
    g:src_setup
    g:yabs-rt
)

PEERDIR(
    kernel/qtree/richrequest
    kernel/qtree/request

    library/cpp/tokenizer
)

SRCS(
    normalize.cpp
)

END()
