LIBRARY()

OWNER(
    mvel
    tsimkha
    mbusel
    scowl
    g:base
)

PEERDIR(
    dict/recognize/queryrec
    kernel/stringmatch_tracker
    kernel/translit
    library/cpp/charset
    library/cpp/on_disk/2d_array
    ysite/yandex/common
    kernel/doom/offroad_erf_wad
    kernel/yawklib
    library/cpp/string_utils/quote
)

SRCS(
    url_sequences.cpp
    urlseq_writer.cpp
)

END()
