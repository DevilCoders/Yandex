JTEST()

JDK_VERSION(11)

OWNER(g:passport_infra)

PEERDIR(
    contrib/java/junit/junit/4.12
    contrib/java/org/slf4j/slf4j-simple
    devtools/jtest
    library/java/tvmauth/dynamic_dst
)

JAVA_SRCS(
    SRCDIR
    ${ARCADIA_ROOT}/library/java/tvmauth/dynamic_dst/src/test
    **/*
)

JAVA_SRCS(simplelogger.properties)

DATA(arcadia/library/cpp/tvmauth/client/ut/files)

# Added automatically to remove dependency on default contrib versions
DEPENDENCY_MANAGEMENT(
    contrib/java/org/slf4j/slf4j-simple/1.8.0-alpha2
)

LINT(extended)

END()
