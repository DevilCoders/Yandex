JAVA_LIBRARY()

JDK_VERSION(11)

OWNER(g:passport_infra)

PEERDIR(
    library/java/tvmauth
)

JAVA_SRCS(
    SRCDIR
    src/main/java
    **/*
)

LINT(extended)

END()

IF (NOT SANITIZER_TYPE)
    RECURSE_FOR_TESTS(
        src/ut
    )
ENDIF()
