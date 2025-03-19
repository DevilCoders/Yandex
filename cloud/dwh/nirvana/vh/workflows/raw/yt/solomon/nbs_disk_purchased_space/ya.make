OWNER(g:cloud-dwh)

PY3_LIBRARY()

PEERDIR(
    nirvana/valhalla/src
)

PY_SRCS(__init__.py)

RESOURCE(
    cloud/dwh/nirvana/vh/workflows/raw/yt/solomon/nbs_disk_purchased_space/resources/parameters.yaml raw/yt/solomon/nbs_disk_purchased_space/resources/parameters.yaml
)

END()
