OWNER(g:logbroker)

NEED_CHECK()

JAVA_LIBRARY(awssdk-extensions-java)

IF(JDK_VERSION == "")
    JDK_VERSION(11)
ENDIF()

LINT(extended)

CHECK_JAVA_DEPS(yes)

JAVA_SRCS(
    SRCDIR src/main/java **/*
)

PEERDIR(
    library/java/tvmauth
    contrib/java/com/amazonaws/aws-java-sdk-core
)

# Added automatically to remove dependency on default contrib versions
DEPENDENCY_MANAGEMENT(
    contrib/java/com/amazonaws/aws-java-sdk-core/1.11.792
)

END()

RECURSE_FOR_TESTS(
    src/test
)
