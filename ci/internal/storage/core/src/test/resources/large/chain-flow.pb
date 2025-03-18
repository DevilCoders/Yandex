# proto-file: ci/proto/storage_api.proto
# proto-message: ExtendedStartFlowRequest

request: {
  flow_process_id: {
    dir: "autocheck/large-tests",
    id: "large-flow"
  }
  branch: "pr:12345",
  revision: {
    commit_id: "right"
  },
  config_revision: {
    hash: "latest"
    branch: "trunk",
    number: 0,
    pull_request_id: 0
  }
}