# proto-file: ci/proto/storage/dovecote.proto
# proto-message: Event

first_fail_tests_event {
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
    iteration {
        id {
            check_id: "100000000000"
            check_type: FULL
            number: 1
        }
        status: RUNNING
    }
}
