OWNER(g:cloud-dwh)

PY3_LIBRARY()

PEERDIR(
    nirvana/valhalla/src
)

PY_SRCS(__init__.py)

RESOURCE(
    cloud/dwh/nirvana/vh/workflows/export/cdm/yt/ch/dm_yc_consumption/resources/parameters.yaml export/cdm/yt/ch/dm_yc_consumption/resources/parameters.yaml
)

END()
