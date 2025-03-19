OWNER(g:cloud-dwh)

PY3_LIBRARY()

PEERDIR(
    nirvana/valhalla/src
)

PY_SRCS(__init__.py)

RESOURCE(
    cloud/dwh/nirvana/vh/workflows/cdm/yt/dm_ba_crm_tags/resources/query.sql cdm/yt/dm_ba_crm_tags/resources/query.sql
    cloud/dwh/nirvana/vh/workflows/cdm/yt/dm_ba_crm_tags/resources/parameters.yaml cdm/yt/dm_ba_crm_tags/resources/parameters.yaml
)

END()
