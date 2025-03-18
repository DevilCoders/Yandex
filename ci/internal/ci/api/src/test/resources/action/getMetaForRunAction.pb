# proto-file: ci/proto/frontend_on_commit_flow_launch_api.proto
# proto-message: GetMetaForRunActionResponse

flow {
    flow_process_id {
        id: "some-action"
    }
    title: "Some flow"
    flowVarsUi {
        schema: "{\"title\":\"Custom launch parameters\",\"type\":\"object\"}"
    }
}
