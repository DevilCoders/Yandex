LIBRARY()

OWNER(
    divankov
    g:snippets
)

SRCS(
    stopword.cpp
)

PEERDIR(
    library/cpp/resource
)

# Proper source is likely svn+ssh://arcadia.yandex.ru/arc/trunk/arcadia_tests_data/wizard/language/stopword.lst
RESOURCE(
    stopword.lst /stopword
)

END()
