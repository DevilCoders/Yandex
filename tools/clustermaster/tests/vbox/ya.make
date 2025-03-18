#SET(SANITIZER_TYPE address)
DLL(vbox 1 0)

OWNER(g:kwyt)

ALLOCATOR(LF)

PEERDIR(
    library/cpp/deprecated/atomic
)

SRCS(
    vbox_common.cpp
    vbox_net.cpp
    vbox_net_wrapper.cpp
    vbox_fs.cpp
    vbox_fs_wrapper.cpp
    vboxps.cpp
)

END()
