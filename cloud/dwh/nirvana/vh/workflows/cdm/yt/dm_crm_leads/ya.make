OWNER(g:cloud-dwh)

PY3_LIBRARY()

PEERDIR(
    nirvana/valhalla/src
)

PY_SRCS(__init__.py)

RESOURCE(
    cloud/dwh/nirvana/vh/workflows/cdm/yt/dm_crm_leads/resources/parameters.yaml cdm/yt/dm_crm_leads/resources/parameters.yaml
    cloud/dwh/nirvana/vh/workflows/cdm/yt/dm_crm_leads/resources/query.sql cdm/yt/dm_crm_leads/resources/query.sql
    cloud/dwh/nirvana/vh/workflows/cdm/yt/dm_crm_leads/resources/check_empty.sql cdm/yt/dm_crm_leads/resources/check_empty.sql
)

END()
