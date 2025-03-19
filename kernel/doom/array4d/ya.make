LIBRARY()

OWNER(
    elric
    g:base
)

SRCS(
    array4d_io.h
    dummy.cpp
)

PEERDIR(
    kernel/indexann_data
    library/cpp/on_disk/4d_array
    kernel/doom/hits
    kernel/doom/progress
    kernel/indexann/interface
)

END()
