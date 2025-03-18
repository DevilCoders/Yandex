LIBRARY()

OWNER(mowgli)

SRCS(
    mtp_jobs.cpp
    multitask.cpp
    queue.cpp
    work_stealing.cpp
)

PEERDIR(
    library/cpp/deprecated/atomic
)

END()
