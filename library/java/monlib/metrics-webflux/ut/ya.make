JTEST()

JDK_VERSION(11)

OWNER(g:solomon)

JAVA_SRCS(
    PACKAGE_PREFIX ru.yandex.monlib.metrics.webflux
    SRCDIR src
    **/*
)

PEERDIR(
    library/java/monlib/metrics
    library/java/monlib/metrics-webflux
    contrib/java/junit/junit/4.12
    devtools/jtest
)

LINT(base)
END()
