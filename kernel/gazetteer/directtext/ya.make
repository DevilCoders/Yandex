LIBRARY()

OWNER(udovichenko-r)

PEERDIR(
    kernel/gazetteer
    kernel/indexer/direct_text
    kernel/lemmer/dictlib
    kernel/search_types
    library/cpp/deprecated/iter
    library/cpp/langmask
)

SRCS(
    dt_gztinput.cpp
)

END()
