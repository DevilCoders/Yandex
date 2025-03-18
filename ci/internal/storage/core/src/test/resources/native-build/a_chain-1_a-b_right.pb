# proto-file: ci/proto/storage/storage_api.proto
# proto-message: LargeTestJob

id {
    iteration_id {
        check_id: "1"
        check_type: HEAVY
    }
}
title: "Native Build Right"
right: true
precommit: true
target: "a/b"
native_target: "some_dir/a"
native_specification: "{\"targets\":{\"a/b\":{\"hid\":\"12274298008748212815\"}}}"
test_info: "{\"toolchain\":\"chain-1\",\"tags\":[\"t\"],\"suite_name\":\"java\",\"suite_id\":\"360acdedbb0c272602dbb2f31ed15913\",\"suite_hid\":7071603783875091617,\"requirements\":{},\"owners\":{},\"size\":\"large\"}"
test_info_source {
    toolchain: "chain-1"
    tags: "t"
    suite_name: "java"
    suite_id: "360acdedbb0c272602dbb2f31ed15913"
    suite_hid: 7071603783875091617
}
check_task_type: CTT_NATIVE_BUILD
arcadia_url: "arcadia-arc:/#right"
arcadia_base: "400"
arcadia_patch: "zipatch:https://zipatch"
distbuild_priority {
    priority_revision: 123
}
