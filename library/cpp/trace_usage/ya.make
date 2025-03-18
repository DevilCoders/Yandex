LIBRARY()

OWNER(kostik)

SRCS(
    context.cpp
    event_processor.cpp
    function_scope.cpp
    global_registry.cpp
    graph_builder.cpp
    logger.cpp
    logreader.cpp
    memory_registry.cpp
    trace_registry.cpp
    traced_guard.cpp
    wait_scope.cpp
)

PEERDIR(
    library/cpp/inf_buffer
    library/cpp/trace_usage/protos
)

END()

RECURSE_FOR_TESTS(
    ut
)
