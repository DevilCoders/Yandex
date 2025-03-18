JAVA_PROGRAM(ci-storage-tests)

OWNER(g:ci)

INCLUDE(${ARCADIA_ROOT}/ci/common/ci-includes-test.inc)

JAVA_SRCS(SRCDIR src/main/java **/*)
JAVA_SRCS(SRCDIR src/main/resources **/*)

PEERDIR(
    ci/internal/storage/reader
    ci/internal/storage/shard
    ci/internal/storage/post-processor
)

FORK_TESTS()
SPLIT_FACTOR(4)

END()

RECURSE_FOR_TESTS(src/test)

