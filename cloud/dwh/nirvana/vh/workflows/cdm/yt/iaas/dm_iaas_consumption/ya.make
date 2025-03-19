OWNER(g:cloud-dwh)

PY3_LIBRARY()

PEERDIR(
    nirvana/valhalla/src
)

PY_SRCS(__init__.py)

RESOURCE(
    cloud/dwh/nirvana/vh/workflows/cdm/yt/iaas/dm_iaas_consumption/resources/query.sql cdm/yt/iaas/dm_iaas_consumption/resources/query.sql
    cloud/dwh/nirvana/vh/workflows/cdm/yt/iaas/dm_iaas_consumption/resources/parameters.yaml cdm/yt/iaas/dm_iaas_consumption/resources/parameters.yaml
)

END()
