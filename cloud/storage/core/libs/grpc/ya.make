LIBRARY()

OWNER(g:cloud-nbs)

SRCS(
    completion.cpp
    credentials.cpp
    executor.cpp
    initializer.cpp
    keepalive.cpp
    logging.cpp
    threadpool.cpp
    time.cpp
)

ADDINCL(
    contrib/libs/grpc
)

NO_COMPILER_WARNINGS()

PEERDIR(
    library/cpp/grpc/common
    library/cpp/logger
    contrib/libs/grpc
    contrib/libs/grpc/grpc
    contrib/libs/grpc/src/proto/grpc/reflection/v1alpha
    library/cpp/deprecated/atomic
)

END()
