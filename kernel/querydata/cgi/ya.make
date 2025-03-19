LIBRARY()

OWNER(
    alexbykov
    mvel
    velavokr
)

GENERATE_ENUM_SERIALIZATION(qd_cgi_strings.h)

SRCS(
    qd_cgi.cpp
    qd_cgi_strings.cpp
    qd_cgi_utils.cpp
    qd_docitems.cpp
    qd_request.cpp
)

PEERDIR(
    library/cpp/string_utils/base64
    kernel/querydata/common
    kernel/querydata/idl/scheme
    kernel/urlid
    kernel/urlnorm
    kernel/hosts/owner
    library/cpp/json
    library/cpp/scheme
    library/cpp/string_utils/relaxed_escaper
    library/cpp/streams/lz
    library/cpp/cgiparam
    library/cpp/string_utils/quote
    library/cpp/string_utils/url
)

END()
