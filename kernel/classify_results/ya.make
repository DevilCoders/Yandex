OWNER(
    epar
    finder
)

LIBRARY()

SRCS(
    snippet_reader.cpp
    snippet_classifier.cpp
    tags.cpp
    classify.cpp
    script.cpp
    regex_wrapper.cpp
    matchable.cpp
)

PEERDIR(
    kernel/lemmer/alpha
    kernel/yawklib
    library/cpp/binsaver
    library/cpp/charset
    library/cpp/containers/comptrie
    library/cpp/containers/ext_priority_queue
    library/cpp/packers
    library/cpp/regex/pire
    library/cpp/scheme
    library/cpp/string_utils/url
    ysite/yandex/doppelgangers
)

END()
