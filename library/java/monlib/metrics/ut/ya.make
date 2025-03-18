JTEST()

JDK_VERSION(11)

OWNER(g:solomon)

JAVA_SRCS(
    PACKAGE_PREFIX ru.yandex.monlib.metrics
    SRCDIR src
    **/*
)

PEERDIR(
    library/java/monlib/metrics
    contrib/java/junit/junit/4.12
    contrib/java/org/openjdk/jol/jol-core/0.9
    devtools/jtest
)

JVM_ARGS(
    -Dru.yandex.solomon.LabelValidator=strict
)

LINT(base)
END()
