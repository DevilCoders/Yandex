LIBRARY()

OWNER(
    d-dima
    g:hollywood
)

PEERDIR(
    alice/hollywood/library/base_scenario
    alice/hollywood/library/dispatcher/common_handles/scenario_handles
    alice/hollywood/library/dispatcher/common_handles/util
    alice/hollywood/library/fast_data
    alice/hollywood/library/frame
    alice/hollywood/library/framework/proto
    alice/hollywood/library/framework/core/codegen
    alice/hollywood/library/global_context
    alice/hollywood/library/metrics
    alice/hollywood/library/nlg
    alice/hollywood/library/request
    alice/library/client
    alice/library/geo
    alice/library/json
    alice/library/logger
    alice/memento/proto
    alice/library/metrics
    alice/library/proto
    alice/library/restriction_level
    alice/library/scenarios/data_sources
    alice/library/sys_datetime
    alice/library/util
    alice/megamind/protos/common
    alice/megamind/protos/scenarios
    apphost/api/service/cpp
    contrib/libs/protobuf
    library/cpp/json
    library/cpp/langs
    library/cpp/protobuf/json
    library/cpp/timezone_conversion
)

SRCS(
    default_renders.cpp
    node_caller.cpp
    nlg_helper.cpp
    scenario_factory.cpp
    scene_graph.cpp
    error.cpp
    render.cpp
    render_impl.cpp
    request.cpp
    run_features.cpp
    scenario.cpp
    scene_base.cpp
    scene.cpp
    request_flags.cpp
    request_helper.cpp
    return_types.cpp
    source.cpp
    setup.cpp
    storage.cpp
    semantic_frames.cpp
    scenario_baseinit.cpp
)

GENERATE_ENUM_SERIALIZATION(scenario_baseinit.h)
GENERATE_ENUM_SERIALIZATION(error.h)

END()
