JAVA_LIBRARY(ci-core)

JAVA_SRCS(SRCDIR src/main/java **/*)
JAVA_SRCS(SRCDIR src/main/resources **/*)

IDEA_RESOURCE_DIRS(src/main/resources/local)

INCLUDE(${ARCADIA_ROOT}/ci/common/ci-includes.inc)

PEERDIR(
    ci/config-schema
    ci/common/application-profiles
    ci/common/utils

    ci/clients/abc
    ci/clients/arcanum
    ci/clients/grpc
    ci/clients/http-client-base
    ci/clients/logs
    ci/clients/pci-express
    ci/clients/sandbox
    ci/clients/taskletv2

    ci/internal/common/ydb
    ci/internal/common/temporal

    ci/proto
    ci/proto/storage
    ci/tasklet/common/proto

    library/java/annotations
    arcanum/events
    arc/api/public
    iceberg/misc-bender-annotations

    contrib/java/com/fasterxml/jackson/core/jackson-databind
    contrib/java/com/fasterxml/jackson/datatype/jackson-datatype-jsr310

    contrib/java/com/github/java-json-tools/json-schema-validator
    contrib/java/org/springframework/spring-context
    contrib/java/io/micrometer/micrometer-registry-prometheus
    contrib/java/io/burt/jmespath-gson
)

END()

RECURSE_FOR_TESTS(
    src/test
)
