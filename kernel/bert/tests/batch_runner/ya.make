PROGRAM()

OWNER(
    g:neural-search
    e-shalnov
    lexeyo
)

PEERDIR(
    kernel/bert
    kernel/bert/tests/batch_runner/protos
    library/cpp/getoptpb
    library/cpp/getoptpb/proto
    library/cpp/time_provider
    library/cpp/threading/future
)

ENABLE(USE_ARCADIA_LIBM)

SRCS(
    main.cpp
)

END()
