UNITTEST_FOR(kernel/doom/offroad_doc_wad)

OWNER(
    elric
    g:base
)

SRCS(
    offroad_doc_wad_ut.cpp
)

PEERDIR(
    library/cpp/digest/md5
    kernel/doom/algorithm
    kernel/doom/wad
    kernel/doom/offroad_wad
)

END()
