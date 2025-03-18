OWNER(g:logbroker)

NEED_CHECK()

JTEST()

JDK_VERSION(11)

LINT(extended)
CHECK_JAVA_DEPS(yes)

JAVA_SRCS(SRCDIR java **/*)

PEERDIR(
    library/java/awssdk-extensions
    contrib/java/com/amazonaws/aws-java-sdk-sqs
    contrib/java/ch/qos/logback/logback-classic
)

# Added automatically to remove dependency on default contrib versions
DEPENDENCY_MANAGEMENT(
    contrib/java/com/amazonaws/aws-java-sdk-sqs/1.11.792
    contrib/java/ch/qos/logback/logback-classic/1.2.3
)

END()
