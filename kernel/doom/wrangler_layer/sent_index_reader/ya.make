LIBRARY()

OWNER(
    lagrunge
    g:base
)

PEERDIR(
    ysite/yandex/erf
    kernel/sent_lens
    kernel/doom/progress
    kernel/doom/offroad_sent_wad_accessor
)

SRCS(
    sent_reader.h
)

END()
