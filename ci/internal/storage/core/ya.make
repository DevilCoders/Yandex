JAVA_LIBRARY(ci-storage-core)

JAVA_SRCS(SRCDIR src/main/java **/*)
JAVA_SRCS(SRCDIR src/main/resources **/*)

INCLUDE(${ARCADIA_ROOT}/ci/common/ci-includes.inc)

PEERDIR(
    ci/internal/common/bazinga
    ci/internal/common/logbroker
    ci/internal/common/temporal
    ci/internal/ci/core

    ci/clients/ayamler
    ci/clients/ci
    ci/clients/old-ci
    ci/clients/testenv

    ci/proto/storage
    ci/proto/event

    yt/java/ytclient

    contrib/java/ru/yandex/clickhouse/clickhouse-jdbc
    contrib/java/me/dinowernli/java-grpc-prometheus
)

EXCLUDE(
    contrib/java/net/jpountz/lz4/lz4
)

END()


RECURSE_FOR_TESTS(
    src/test
)

