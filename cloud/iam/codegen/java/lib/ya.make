JAVA_LIBRARY(yandex-cloud-constants)

JDK_VERSION(11)

OWNER(g:cloud-iam)

JAVA_SRCS(SRCDIR ${BINDIR}/generated **/*)

RUN_JAVA_PROGRAM(
    yandex.cloud.iam.FixtureCodegen
    --output-dir ${BINDIR}/generated
    OUT_DIR generated
    CLASSPATH cloud/iam/codegen/java/codegen
)

END()

RECURSE_FOR_TESTS(
    ut
)
