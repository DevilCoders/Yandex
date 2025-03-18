LIBRARY()

OWNER(
    g:base
    g:middle
)

SRCS(
    async_task_batch.cpp
)

PEERDIR(
    library/cpp/deprecated/atomic
)

END()

NEED_CHECK()
