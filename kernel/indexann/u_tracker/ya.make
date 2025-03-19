LIBRARY()

OWNER(g:indexann)

PEERDIR(
    kernel/factor_storage
    kernel/indexann_data
    kernel/u_tracker
)

SRCS(
    wrapper.cpp
)

END()
