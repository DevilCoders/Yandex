# proto-file: ci/proto/event/tests.proto
# proto-message: Event

check_finished_event {
    review_request {
        id: 42
    }
    status: COMPLETED_SUCCESS
    author {
        name: "firov"
    }
    check {
        id: 100000000000
        status: COMPLETED_SUCCESS
    }
}
