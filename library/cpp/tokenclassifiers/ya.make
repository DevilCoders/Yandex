OWNER(g:cpp-contrib)

LIBRARY()

SRCS(
    token_classifiers.cpp
    token_classifiers_singleton.cpp
    token_types.cpp
    token_markup.cpp
    classifiers/email_classifier.cpp
    classifiers/url_classifier.cpp
    classifiers/punycode_classifier.cpp
)

PEERDIR(
    contrib/libs/libidn
    library/cpp/regex/pire
    library/cpp/token
    library/cpp/deprecated/atomic
)

END()
