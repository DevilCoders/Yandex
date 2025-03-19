OWNER(g:cloud-nbs)

RECURSE_ROOT_RELATIVE(
    cloud/blockstore/daemon

    cloud/compute/snapshot/cmd/yc-snapshot
    cloud/compute/snapshot/cmd/yc-snapshot-populate-database

    cloud/disk_manager/cmd/yc-disk-manager
    cloud/disk_manager/cmd/yc-disk-manager-admin
    cloud/disk_manager/cmd/yc-disk-manager-init-db
    cloud/disk_manager/test/mocks/accessservice

    kikimr/driver
    kikimr/tools/cfg
)
