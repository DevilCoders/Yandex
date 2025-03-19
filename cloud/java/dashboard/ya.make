JAVA_PROGRAM()

JDK_VERSION(11)

OWNER(
    ssytnik
)

PEERDIR(
    contrib/java/javax/annotation/javax.annotation-api/1.3.2
    contrib/java/org/apache/logging/log4j/log4j-core/2.11.2
    contrib/java/com/fasterxml/jackson/core/jackson-databind/2.9.7
    contrib/java/com/fasterxml/jackson/dataformat/jackson-dataformat-yaml/2.9.7
    contrib/java/org/apache/httpcomponents/httpclient/4.5.6
    contrib/java/com/github/fge/json-patch/1.9
    contrib/java/org/projectlombok/lombok/1.18.22
    contrib/java/com/google/guava/guava/27.1-jre
    contrib/java/org/jetbrains/annotations/16.0.3
)

JAVA_SRCS(SRCDIR src/main/java **/*)
JAVA_SRCS(SRCDIR src/main/resources **/*)

ANNOTATION_PROCESSOR(
    lombok.launch.AnnotationProcessorHider$AnnotationProcessor
)

CHECK_JAVA_DEPS(yes)

NO_LINT()

END()

RECURSE_FOR_TESTS(src/test)
