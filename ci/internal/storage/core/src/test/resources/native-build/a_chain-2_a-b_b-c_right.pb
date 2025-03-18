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
target: "a/b;b/c"
native_target: "some_dir/a"
native_specification: "{\"targets\":{\"a/b\":{\"hid\":\"11850187821387272670\"},\"b/c\":{\"hid\":\"18002154206543829652\"}}}"
test_info: "{\"toolchain\":\"chain-2\",\"tags\":[\"t\"],\"suite_name\":\"java\",\"suite_id\":\"4c30c55c6258acdda82d03c9013ea943\",\"suite_hid\":229631353628278491,\"requirements\":{},\"owners\":{},\"size\":\"large\"}"
test_info_source {
    toolchain: "chain-2"
    tags: "t"
    suite_name: "java"
    suite_id: "4c30c55c6258acdda82d03c9013ea943"
    suite_hid: 229631353628278491
}
check_task_type: CTT_NATIVE_BUILD
arcadia_url: "arcadia-arc:/#right"
arcadia_base: "400"
arcadia_patch: "zipatch:https://zipatch"
distbuild_priority {
    priority_revision: 123
}
