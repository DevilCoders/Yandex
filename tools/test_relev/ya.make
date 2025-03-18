PROGRAM()

SRCS(
    FetchYaResults.cpp
    ReadAssessData.cpp
    UrlsToAssess.cpp
    YaRelev.cpp
    req_result.cpp
    stdafx.cpp
    test_relev.cpp
    http_fetch.cpp
    SearchDuplicates.cpp
)

PEERDIR(
    kernel/groupattrs
    library/cpp/charset
    library/cpp/getopt
    library/cpp/html/pcdata
    library/cpp/string_utils/old_url_normalize
    library/cpp/uri
    library/cpp/http/io
    library/cpp/string_utils/quote
    library/cpp/deprecated/atomic
)

END()
