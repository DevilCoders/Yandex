OWNER(g:cloud-nbs)

PY3TEST()

TEST_SRCS(
    test_configs.py
)

PEERDIR(
    cloud/disk_manager/internal/pkg/configs/client/config
    cloud/disk_manager/internal/pkg/configs/server/config
    contrib/python/protobuf
)

DATA(arcadia/cloud/disk_manager/configs)

END()
