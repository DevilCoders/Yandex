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
target: "a/b;b/c"
native_target: "some_dir/a"
native_specification: "{\"targets\":{\"a/b\":{\"hid\":\"12274298008748212815\"},\"b/c\":{\"hid\":\"8786328245117240835\"}}}"
test_info: "{\"toolchain\":\"chain-1\",\"tags\":[\"t\"],\"suite_name\":\"java\",\"suite_id\":\"b47f8c4e76dc30274039d40dd26072e3\",\"suite_hid\":1723402840667891407,\"requirements\":{},\"owners\":{},\"size\":\"large\"}"
test_info_source {
    toolchain: "chain-1"
    tags: "t"
    suite_name: "java"
    suite_id: "b47f8c4e76dc30274039d40dd26072e3"
    suite_hid: 1723402840667891407
}
check_task_type: CTT_NATIVE_BUILD
arcadia_url: "arcadia-arc:/#left"
arcadia_base: "200"
distbuild_priority {
    priority_revision: 123
}
