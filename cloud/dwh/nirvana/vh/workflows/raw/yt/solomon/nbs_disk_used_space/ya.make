OWNER(g:cloud-dwh)

PY3_LIBRARY()

PEERDIR(
    nirvana/valhalla/src

    cloud/dwh/utils
)

PY_SRCS(__init__.py)

RESOURCE(
    cloud/dwh/nirvana/vh/workflows/raw/yt/solomon/nbs_disk_used_space/resources/parameters.yaml raw/yt/solomon/nbs_disk_used_space/resources/parameters.yaml
)

END()
