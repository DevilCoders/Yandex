# proto-file: ci/proto/event/tests.proto
# proto-message: Event

iteration_finished_event {
    review_request {
        id: 42
    }
    status: COMPLETED_SUCCESS
    author {
        name: "firov"
    }
    check {
        id: 100000000000
        status: RUNNING
    }
    iteration {
        id {
            check_id: "100000000000"
            check_type: FULL
            number: 1
        }
        status: COMPLETED_SUCCESS
    }
}
