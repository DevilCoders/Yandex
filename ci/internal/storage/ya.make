RECURSE(
    api
    core
    exporter
    post-processor
    reader
    shard
    tests
    tms
    ../observer
    ../integration-tests/storage-observer-tests
)

INCLUDE(${ARCADIA_ROOT}/ci/common/includes/common-test-targets.inc)
