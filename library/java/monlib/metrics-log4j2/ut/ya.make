JTEST()

JDK_VERSION(11)

OWNER(g:solomon)

JAVA_SRCS(
    SRCDIR src
    **/*
)

PEERDIR(
    library/java/monlib/metrics-log4j2
    contrib/java/junit/junit/4.12
    devtools/jtest
)

END()
