LIBRARY()

OWNER(g:megamind)

PEERDIR(
    alice/library/asr_hypothesis_picker
    alice/library/blackbox
    alice/library/experiments
    alice/library/json
    alice/library/logger
    alice/library/metrics
    alice/library/network
    alice/nlu/granet/lib/compiler
    alice/megamind/library/apphost_request
    alice/megamind/library/apphost_request/protos
    alice/megamind/library/biometry
    alice/megamind/library/experiments
    alice/megamind/library/globalctx
    alice/megamind/library/grpc_request
    alice/megamind/library/handlers/speechkit
    alice/megamind/library/kv_saas
    alice/megamind/library/proactivity/common
    alice/megamind/library/registry
    alice/megamind/library/request_composite
    alice/megamind/library/request_composite/client
    alice/megamind/library/requestctx
    alice/megamind/library/saas
    alice/megamind/library/scenarios/protocol
    alice/megamind/library/search
    alice/megamind/library/speechkit
    alice/megamind/library/worldwide/language
    alice/megamind/protos/common
    alice/megamind/protos/grpc_request
    alice/megamind/protos/scenarios
    alice/protos/api/meta

    contrib/libs/googleapis-common-protos

    search/begemot/apphost
    search/begemot/rules/alice/response/proto

    library/cpp/cgiparam
    library/cpp/http/io
    library/cpp/logger
    library/cpp/json
    apphost/api/service/cpp
    apphost/lib/common
    apphost/lib/proto_answers
)

SRCS(
    begemot.cpp
    blackbox.cpp
    combinators.cpp
    components.cpp
    continue_setup.cpp
    fallback_response.cpp
    grpc/common.cpp
    grpc/grpc_finalize_handler.cpp
    grpc/grpc_setup_handler.cpp
    grpc/register.cpp
    misspell.cpp
    node.cpp
    on_utterance.cpp
    personal_intents.cpp
    proactivity.cpp
    polyglot.cpp
    postpone_log_writer.cpp
    query_tokens_stats.cpp
    responses.cpp
    saas.cpp
    skr.cpp
    speechkit_session.cpp
    util.cpp
    stage_timers.cpp
    walker_apply.cpp
    walker_monitoring.cpp
    walker_prepare.cpp
    walker_run.cpp
    walker_util.cpp
    walker.cpp
    websearch.cpp
    websearch_query.cpp
)

END()

RECURSE_FOR_TESTS(
    ut
)
