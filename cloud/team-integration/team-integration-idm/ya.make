OWNER(g:cloud-iam)

JAVA_LIBRARY(team-integration-idm)

INCLUDE(${ARCADIA_ROOT}/cloud/team-integration/ya.make.main.inc)

JAVA_SRCS(SRCDIR src/main/java **/*)
JAVA_SRCS(SRCDIR src/main/resources **/*)

PEERDIR(
    cloud/team-integration/team-integration-abc
    cloud/team-integration/team-integration-cloud-resource-manager-client
    cloud/team-integration/team-integration-team-abc-client
    cloud/team-integration/team-integration-tvm-http

    contrib/java/com/fasterxml/jackson/core/jackson-annotations
    contrib/java/com/fasterxml/jackson/core/jackson-databind
    contrib/java/com/google/protobuf/protobuf-java
    contrib/java/io/grpc/grpc-api
    contrib/java/javax/servlet/javax.servlet-api
    contrib/java/org/apache/logging/log4j/log4j-api
    contrib/java/org/jboss/resteasy/resteasy-jaxrs
    contrib/java/org/jboss/resteasy/resteasy-servlet-initializer
    contrib/java/ru/yandex/cloud/cloud-proto-java
    contrib/java/yandex/cloud/common/library/json-util
    contrib/java/yandex/cloud/common/library/static-di
    contrib/java/yandex/cloud/common/library/util
    contrib/java/yandex/cloud/http-healthcheck
    contrib/java/yandex/cloud/iam-common
    contrib/java/yandex/cloud/iam-remote-operation
)

END()

RECURSE_FOR_TESTS(
    src/test
)
