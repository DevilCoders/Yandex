LIBRARY(mango-lib-common)

OWNER(sashateh)

PEERDIR(
    library/cpp/string_utils/base64

    kernel/mango/proto
    kernel/struct_codegen/reflection
    library/cpp/charset
    library/cpp/html/pdoc
    library/cpp/logger
    library/cpp/on_disk/2d_array
    library/cpp/on_disk/head_ar
    library/cpp/packedtypes
    library/cpp/regex/pire
    util/draft
)

SRCS(
    constraints.cpp
    html.cpp
    log.cpp
    portion_tags.cpp
    quotes.cpp
    tools.cpp
    types.cpp
    types_format.cpp
)

STRUCT_CODEGEN(types_format_gen)

END()
