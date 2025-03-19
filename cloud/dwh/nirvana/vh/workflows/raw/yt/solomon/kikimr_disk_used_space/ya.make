OWNER(g:cloud-dwh)

PY3_LIBRARY()

PEERDIR(
    nirvana/valhalla/src
)

PY_SRCS(__init__.py)

RESOURCE(
    cloud/dwh/nirvana/vh/workflows/raw/yt/solomon/kikimr_disk_used_space/resources/parameters.yaml raw/yt/solomon/kikimr_disk_used_space/resources/parameters.yaml
)

END()
