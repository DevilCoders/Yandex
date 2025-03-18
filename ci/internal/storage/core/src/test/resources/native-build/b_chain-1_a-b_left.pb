# proto-file: ci/proto/storage/storage_api.proto
# proto-message: LargeTestJob

id {
    iteration_id {
        check_id: "1"
        check_type: HEAVY
    }
}
title: "Native Build Left"
right: false
precommit: true
target: "a/b"
native_target: "some_dir/b"
native_specification: "{\"targets\":{\"a/b\":{\"hid\":\"12274298008748212815\"}}}"
test_info: "{\"toolchain\":\"chain-1\",\"tags\":[\"t\"],\"suite_name\":\"java\",\"suite_id\":\"2c7a0239ebde057f5d902d858a1b764b\",\"suite_hid\":12290385759804992777,\"requirements\":{},\"owners\":{},\"size\":\"large\"}"
test_info_source {
    toolchain: "chain-1"
    tags: "t"
    suite_name: "java"
    suite_id: "2c7a0239ebde057f5d902d858a1b764b"
    suite_hid: 12290385759804992777
}
check_task_type: CTT_NATIVE_BUILD
arcadia_url: "arcadia-arc:/#left"
arcadia_base: "200"
distbuild_priority {
    priority_revision: 123
}
