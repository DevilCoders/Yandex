JAVA_LIBRARY(tvm-client)

JAVA_SRCS(SRCDIR src/main/java **/*)

INCLUDE(${ARCADIA_ROOT}/ci/common/ci-includes.inc)

PEERDIR(
    ci/clients/http-client-base

    contrib/java/ru/yandex/passport/tvmauth-java
    contrib/java/io/grpc/grpc-all
    contrib/java/io/grpc/grpc-services
)
END()

RECURSE_FOR_TESTS(src/test)
