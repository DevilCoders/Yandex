JAVA_LIBRARY(kafka_authorizer)

JDK_VERSION(11)

PEERDIR(
    contrib/java/org/projectlombok/lombok/1.18.20
    contrib/java/org/mockito/mockito-core/3.5.11
    contrib/java/org/apache/kafka/kafka_2.12/2.6.0
)

JAVA_SRCS(
    SRCDIR src/main/java **/*.java
)

NO_LINT()

END()

RECURSE_FOR_TESTS(
    src/test
)
