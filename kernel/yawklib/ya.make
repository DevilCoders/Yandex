OWNER(g:cpp-contrib)

LIBRARY()

SRCS(
    wtrutil.cpp
    declension.cpp
    phrase_iterator.cpp
    phrase_analyzer.cpp
    factories.cpp
    rule.cpp
    script_aliases.cpp
    dupes.cpp
)

PEERDIR(
    kernel/lemmer
    kernel/lemmer/dictlib
    library/cpp/charset
    library/cpp/containers/2d_array
    library/cpp/containers/ext_priority_queue
    library/cpp/containers/rarefied_array
    util/draft
    ysite/yandex/doppelgangers
)

END()
