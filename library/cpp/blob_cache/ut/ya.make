UNITTEST()

OWNER(gotmanov)

PEERDIR(
    ADDINCL library/cpp/blob_cache
    library/cpp/threading/mtp_tasks
)

SRCS(
    paged_blob_hasher_ut.cpp
)

END()
