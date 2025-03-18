PEERDIR(
    contrib/java/org/projectlombok/lombok
)

ANNOTATION_PROCESSOR(
    lombok.launch.AnnotationProcessorHider$AnnotationProcessor
)

JAVA_EXTERNAL_DEPENDENCIES(
    ${ARCADIA_ROOT}/ci/lombok.config
)
