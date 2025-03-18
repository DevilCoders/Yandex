# proto-file: ci/proto/storage/dovecote.proto
# proto-message: Event

you_broke_tests_event {
    owner: "firov"
    author: "firov"
    left_revision {
        branch: "trunk"
        revision: "r1"
        revision_number: 1
        timestamp {
            seconds: 1
        }
    }
    right_revision {
        branch: "trunk"
        revision: "r2"
        revision_number: 2
        timestamp {
            seconds: 2
        }
    }
    check_id: 100000000000
    iteration_type: FULL
    number_of_paths: 1
    paths {
        path: "ci"
        number_of_tests: 1
        tests {
            hid: "1001"
            suite_hid: "1001"
            name: "example_suite"
            result_type: RT_TEST_SUITE_MEDIUM
            link_names: "log"
            link_names: "log"
            link_values: "http://log_1"
            link_values: "http://log_2"
            tags: "tag_1"
            tags: "tag_2"
            toolchains {
                name: "test-toolchain"
                left: TS_OK
                right: TS_FAILED
                diff_type: TDT_FAILED_BROKEN
            }
            old_test_id: "old-suite-id"
        }
    }
    number_of_broken_tests: 1
    right_revision_title: "Test commit"
}
