LIBRARY()

OWNER(g:snippets)

SRCS(
    manip.cpp
    sent.cpp
    segments.cpp
)

PEERDIR(
    kernel/snippets/idl
    kernel/tarc/docdescr
    kernel/tarc/iface
    kernel/tarc/markup_zones
)

END()
