OWNER(g:cloud-nbs)

GO_TEST_FOR(cloud/disk_manager/internal/pkg/grpc)

INCLUDE(${ARCADIA_ROOT}/cloud/disk_manager/internal/pkg/grpc/testcommon/common.inc)

GO_XTEST_SRCS(
    snapshot_service_test.go
)

END()
