UNITTEST_FOR(kernel/doom/offroad_struct_wad)

OWNER(
    noobgam
    g:base
)

SRCS(
    offroad_struct_wad_ut.cpp
    doc_data_holder_ut.cpp
)

PEERDIR(
)

IF (MSVC)
    SIZE(MEDIUM)
ENDIF()

END()

