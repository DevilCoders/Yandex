# proto-file: ci/proto/common.proto
# proto-message: FlowDescription

flow_process_id {
    dir: "ci-tests"
    id: "flow-with-flow-vars"
}
title: "Release frontend flow ${flow-vars.title}"
flowVarsUi {
    schema: "{\"type\":\"object\",\"required\":[\"title\",\"stages\",\"iterations\"],\"properties\":{\"title\":{\"type\":\"string\"},\"stages\":{\"type\":\"object\",\"additionalProperties\":false,\"properties\":{\"testing\":{\"type\":\"boolean\"},\"prestable\":{\"type\":\"boolean\"},\"stable\":{\"type\":\"boolean\"}}},\"iterations\":{\"type\":\"integer\"},\"some-not-required-field\":{\"type\":\"number\"}}}"
}
