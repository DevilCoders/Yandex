OWNER(g:cpp-contrib)

LIBRARY()

SRCS(
    url_canonizer.cpp
    url_variants.cpp
)

PEERDIR(
    contrib/libs/libidn
    kernel/factor_slices
    kernel/hosts/minifilter
    kernel/hosts/owner
    kernel/keyinv/invkeypos
    kernel/mirrors
    kernel/url_tools
    kernel/urlnorm
    library/cpp/charset
    library/cpp/deprecated/split
    library/cpp/string_utils/old_url_normalize
    library/cpp/string_utils/quote
    library/cpp/string_utils/url
    library/cpp/tld
    library/cpp/uri
    library/cpp/deprecated/atomic
)

END()
