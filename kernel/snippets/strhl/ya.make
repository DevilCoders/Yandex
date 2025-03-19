LIBRARY()

OWNER(g:snippets)

SRCS(
    glue_common.cpp
    goodwrds.cpp
    forms.cpp
    pack.cpp
    zonedstring.cpp
    zonedstring.proto
    zs_transformer.cpp
)

PEERDIR(
    kernel/lemmer
    kernel/qtree/request
    kernel/qtree/richrequest
    kernel/reqerror
    kernel/text_marks
    library/cpp/charset
    library/cpp/containers/atomizer
    library/cpp/containers/str_hash
    library/cpp/token
    library/cpp/tokenizer
)

END()
