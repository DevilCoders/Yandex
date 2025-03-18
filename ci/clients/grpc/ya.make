JAVA_LIBRARY(ci-common-grpc)

JAVA_SRCS(SRCDIR src/main/java **/*)

INCLUDE(${ARCADIA_ROOT}/ci/common/ci-includes.inc)

PEERDIR(
    ci/clients/tvm
    contrib/java/io/grpc/grpc-all
    contrib/java/io/grpc/grpc-services
    contrib/java/com/google/protobuf/protobuf-java-util
    contrib/java/org/slf4j/slf4j-api
    contrib/java/org/apache/commons/commons-lang3
    contrib/java/org/springframework/spring-context
)

END()

RECURSE_FOR_TESTS(src/test)
