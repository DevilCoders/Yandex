JUNIT5()

SIZE(SMALL)

JAVA_SRCS(SRCDIR java **/*)

JVM_ARGS(
    --add-opens=java.base/java.lang.reflect=ALL-UNNAMED
)

PEERDIR(
    tasklet/sdk/v2/java
)

END()
