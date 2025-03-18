JAVA_PROGRAM(ci-storage-observer-tests)

OWNER(g:ci)

INCLUDE(${ARCADIA_ROOT}/ci/common/ci-includes-test.inc)

JAVA_SRCS(SRCDIR src/main/java **/*)

PEERDIR(
    ci/internal/observer/reader
    ci/internal/storage/tests
)

END()

RECURSE_FOR_TESTS(src/test)

