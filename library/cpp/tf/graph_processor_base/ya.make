LIBRARY()

NO_BUILD_IF(OS_WINDOWS)

OWNER(
    dbakshee
    smirnovpavel
    g:danet-dev
)

PEERDIR(
    contrib/libs/tf
    library/cpp/tf/graph_processor_base/graph_def_multiproto
)

SRCS(
    graph_processor_base.cpp
)

IF (TENSORFLOW_WITH_CUDA)
    CFLAGS(
        -DGOOGLE_CUDA=1
    )
ENDIF()

IF (TENSORFLOW_WITH_XLA)
    CFLAGS(
        -DTENSORFLOW_WITH_XLA=1
    )
ENDIF()

END()
