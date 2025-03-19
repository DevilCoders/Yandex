LIBRARY()

OWNER(g:snippets)

SRCS(
    cut.cpp
    special.cpp
)

PEERDIR(
    kernel/snippets/custom/hostnames_data
    kernel/snippets/strhl
    kernel/snippets/urlcut
    kernel/snippets/urlmenu/common
    library/cpp/langs
    library/cpp/string_utils/url
)

END()
