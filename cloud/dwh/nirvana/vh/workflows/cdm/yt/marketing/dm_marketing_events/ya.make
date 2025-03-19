OWNER(g:cloud-dwh)

PY3_LIBRARY()

PEERDIR(
    nirvana/valhalla/src
)

PY_SRCS(__init__.py)

RESOURCE(
    cloud/dwh/nirvana/vh/workflows/cdm/yt/marketing/dm_marketing_events/resources/parameters.yaml cdm/yt/marketing/dm_marketing_events/resources/parameters.yaml
    cloud/dwh/nirvana/vh/workflows/cdm/yt/marketing/dm_marketing_events/resources/query.sql cdm/yt/marketing/dm_marketing_events/resources/query.sql
)

END()
