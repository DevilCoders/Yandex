LIBRARY()

OWNER(g:megamind)

PEERDIR(
    alice/library/blackbox
    alice/library/frame
    alice/library/geo
    alice/library/iot
    alice/library/json
    alice/library/logger
    alice/library/metrics
    alice/library/restriction_level
    alice/library/unittest
    alice/megamind/library/apphost_request
    alice/megamind/library/classifiers/formulas
    alice/megamind/library/config
    alice/megamind/library/config/protos
    alice/megamind/library/context
    alice/megamind/library/experiments
    alice/megamind/library/factor_storage
    alice/megamind/library/globalctx
    alice/megamind/library/memento
    alice/megamind/library/models/directives
    alice/megamind/library/modifiers
    alice/megamind/library/new_modifiers
    alice/megamind/library/proactivity/common
    alice/megamind/library/registry
    alice/megamind/library/request
    alice/megamind/library/request_composite
    alice/megamind/library/request_composite/client
    alice/megamind/library/requestctx
    alice/megamind/library/response
    alice/megamind/library/scenarios/config_registry
    alice/megamind/library/scenarios/helpers/interface
    alice/megamind/library/scenarios/interface
    alice/megamind/library/session
    alice/megamind/library/sources
    alice/megamind/library/speechkit
    alice/megamind/library/stage_wrappers
    alice/megamind/library/util
    alice/megamind/library/walker
    alice/megamind/protos/scenarios
    alice/megamind/protos/speechkit
    apphost/lib/proto_answers
    apphost/lib/service_testing
    catboost/libs/model
    dj/services/alisa_skills/server/proto/client
    kernel/catboost
    kernel/factor_slices
    kernel/geodb
    library/cpp/geobase
    library/cpp/http/io
    library/cpp/json
    library/cpp/logger
    library/cpp/resource
    library/cpp/scheme
    library/cpp/testing/gmock_in_unittest
    library/cpp/testing/unittest
    library/cpp/timezone_conversion
)

SRCS(
    apphost_helpers.cpp
    components.cpp
    fake_guid_generator.cpp
    fake_modifier.cpp
    fake_registry.cpp
    mock_context.cpp
    mock_data_sources.cpp
    mock_global_context.cpp
    mock_guid_generator.cpp
    mock_http_response.cpp
    mock_modifier_request_factory.cpp
    mock_postclassify_state.cpp
    mock_request_context.cpp
    mock_response_builder.cpp
    mock_responses.cpp
    mock_scenario_wrapper.cpp
    mock_session.cpp
    mock_walker_request.cpp
    modifier_fixture.cpp
    request_fixture.cpp
    response.cpp
    speechkit.cpp
    speechkit_api_builder.cpp
    utils.cpp

    test.proto
)

END()
