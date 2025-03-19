JTEST()

JDK_VERSION(11)

OWNER(
    ssytnik
)

JAVA_SRCS(SRCDIR java **/*)
JAVA_SRCS(SRCDIR resources **/*)

SIZE(SMALL)

PEERDIR(
    cloud/java/dashboard
)

PEERDIR(
    contrib/java/junit/junit/4.12
    contrib/java/org/assertj/assertj-core/3.11.1
    contrib/java/org/projectlombok/lombok/1.18.22
    contrib/java/javax/annotation/javax.annotation-api/1.3.2
    contrib/java/org/jetbrains/annotations/16.0.3
)

ANNOTATION_PROCESSOR(
    lombok.launch.AnnotationProcessorHider$AnnotationProcessor
)

NO_LINT()

END()
