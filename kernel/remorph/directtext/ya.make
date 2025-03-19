LIBRARY()

OWNER(g:remorph)

SRCS(
    directtext.cpp
    dt_breaksegment.cpp
    dt_factory.cpp
    dt_input_symbol.cpp
    dt_processor.cpp
    dt_segment.cpp
)

PEERDIR(
    kernel/gazetteer
    kernel/gazetteer/directtext
    kernel/indexer/direct_text
    kernel/lemmer
    kernel/remorph/cascade
    kernel/remorph/common
    kernel/remorph/core
    kernel/remorph/facts
    kernel/remorph/input
    kernel/search_types
    library/cpp/charset
    library/cpp/containers/sorted_vector
    library/cpp/langmask
    library/cpp/langs
    library/cpp/token
    library/cpp/wordpos
)

END()
