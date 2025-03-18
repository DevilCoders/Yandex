JAVA_PROGRAM(metrics-push)

JDK_VERSION(11)
MAVEN_GROUP_ID(ru.yandex.monlib.examples)

OWNER(g:solomon)

SET(slf4j_version 1.8.0-alpha2)
SET(log4j_version 2.11.0)

JAVA_SRCS(
    PACKAGE_PREFIX ru.yandex.monlib.metrics.example.push
    SRCDIR src
    **/*.java
)

PEERDIR(
    library/java/monlib/metrics
    library/java/monlib/metrics-jvm
    library/java/monlib/metrics-examples/metrics-data-model
    # 3rd-party dependency

    contrib/java/org/asynchttpclient/async-http-client/2.10.4
)

LINT(base)
END()
