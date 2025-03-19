OWNER(g:cloud-iam)

JAVA_LIBRARY(team-integration-tvm-grpc)

INCLUDE(${ARCADIA_ROOT}/cloud/team-integration/ya.make.main.inc)

JAVA_SRCS(SRCDIR src/main/java **/*)

PEERDIR(
    contrib/java/yandex/cloud/iam-common
    intranet/d/backend/service/proto
)

END()
