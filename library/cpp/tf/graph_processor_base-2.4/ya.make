LIBRARY()

NO_BUILD_IF(OS_WINDOWS)

OWNER(
    dbakshee
    smirnovpavel
    g:danet-dev
)

IF (TENSORFLOW_OLD)
    ADDINCL(GLOBAL library/cpp/tf)

    PEERDIR(
        library/cpp/tf/graph_processor_base
    )

ELSE()
    ADDINCL(GLOBAL library/cpp/tf/graph_processor_base-2.4)

    PEERDIR(
        contrib/libs/tf-2.4
        library/cpp/tf/graph_processor_base-2.4/graph_def_multiproto
    )

    SRCS(
        graph_processor_base.cpp
    )

ENDIF()

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
