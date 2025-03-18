JAVA_PROGRAM()

OWNER(g:ci)

UBERJAR()

JAVA_SRCS(SRCDIR src/main/java **/*.java)

PEERDIR(
    tasklet/sdk/v2/java
    ci/tasklet/registry/demov2/woodflow/sawmill/proto
)

END()

RECURSE(
    proto
)

RECURSE_FOR_TESTS(
    src/test
)
