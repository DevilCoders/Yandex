UNITTEST()

OWNER(pozhilov)

PEERDIR(
    library/cpp/nirvana/job_context
)

SRCS(
    job_context_ut.cpp
)

DATA(
    arcadia/library/cpp/nirvana/job_context/ut/data
    arcadia/library/cpp/nirvana/job_context/ut/mr_data
)

END()
