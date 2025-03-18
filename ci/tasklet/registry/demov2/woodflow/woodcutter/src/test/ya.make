JUNIT5()

SIZE(SMALL)

JAVA_SRCS(SRCDIR java **/*)
JAVA_SRCS(SRCDIR resources **/*)

JVM_ARGS(
    --add-opens=java.base/java.lang.reflect=ALL-UNNAMED
)

PEERDIR(
    ci/tasklet/registry/demov2/woodflow/common/src/test
    ci/tasklet/registry/demov2/woodflow/woodcutter
)

END()
