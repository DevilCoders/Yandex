
OWNER(g:cloud-dwh)

PY3_LIBRARY()

PEERDIR(
    nirvana/valhalla/src
)

PY_SRCS(__init__.py)

RESOURCE(
    cloud/dwh/nirvana/vh/workflows/raw/yt/solomon/yandexcloud_cpu_usage/resources/parameters.yaml raw/yt/solomon/yandexcloud_cpu_usage/resources/parameters.yaml
)

END()
