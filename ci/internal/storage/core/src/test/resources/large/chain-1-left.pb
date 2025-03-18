# proto-file: ci/proto/storage/storage_api.proto
# proto-message: LargeTestJob

id {
    iteration_id {
        check_id: "1"
        check_type: HEAVY
    }
}
title: "Large Left"
right: false
precommit: true
target: "a/b/c"
test_info: "{\"toolchain\":\"chain-1\",\"tags\":[\"t\"],\"suite_name\":\"java\",\"suite_id\":\"02f327ca347f486b087713a01b51e115\",\"suite_hid\":9223372036854775808,\"requirements\":{\"sb_vault\":\"token-1\",\"ram\":32},\"owners\":{},\"size\":\"large\"}"
test_info_source: {
    toolchain: "chain-1"
    tags: ["t"]
    suite_name: "java"
    suite_id: "02f327ca347f486b087713a01b51e115"
    suite_hid: 9223372036854775808
}
check_task_type: CTT_LARGE_TEST
arcadia_url: "arcadia-arc:/#left"
arcadia_base: "200"
distbuild_priority {
    priority_revision: 123
}
