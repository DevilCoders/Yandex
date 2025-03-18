# proto-file: ci/proto/event/tests.proto
# proto-message: Event

check_created_event {
    review_request {
        id: 42
    }
    author {
        name: "firov"
    }
    check {
        id: 100000000000
        status: RUNNING
    }
}
