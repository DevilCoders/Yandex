JAVA_PROGRAM()

JDK_VERSION(11)

SET(jackson_version 2.10.3)
SET(lombok-version 1.18.22)
SET(spring-version 5.2.6.RELEASE)
SET(jcommander-version 1.72)
SET(freemarker-version 2.3.23)
SET(guava-version 28.1-jre)

OWNER(g:cloud-iam)

JAVA_SRCS(SRCDIR src/main/java **/*)
JAVA_SRCS(SRCDIR src/main/resources **/*)

PEERDIR(
    cloud/iam/codegen/java/fixtures

    contrib/java/org/projectlombok/lombok

    contrib/java/com/beust/jcommander
    contrib/java/org/springframework/spring-core

    contrib/java/com/fasterxml/jackson/core/jackson-core
    contrib/java/com/fasterxml/jackson/core/jackson-annotations
    contrib/java/com/fasterxml/jackson/core/jackson-databind
    contrib/java/com/fasterxml/jackson/dataformat/jackson-dataformat-yaml

    contrib/java/org/freemarker/freemarker
    contrib/java/com/google/guava/guava
)

DEPENDENCY_MANAGEMENT(
    contrib/java/com/fasterxml/jackson/core/jackson-core/${jackson_version}
    contrib/java/com/fasterxml/jackson/core/jackson-annotations/${jackson_version}
    contrib/java/com/fasterxml/jackson/core/jackson-databind/${jackson_version}
    contrib/java/com/fasterxml/jackson/dataformat/jackson-dataformat-yaml/${jackson_version}

    contrib/java/org/springframework/spring-core/${spring-version}
    contrib/java/org/projectlombok/lombok/${lombok-version}

    contrib/java/com/beust/jcommander/${jcommander-version}
    contrib/java/org/freemarker/freemarker/${freemarker-version}
    contrib/java/com/google/guava/guava/${guava-version}
)


ANNOTATION_PROCESSOR(
    lombok.launch.AnnotationProcessorHider$AnnotationProcessor
)

CHECK_JAVA_DEPS(yes)

LINT(base)
END()
