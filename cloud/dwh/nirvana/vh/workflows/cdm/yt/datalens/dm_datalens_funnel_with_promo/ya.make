OWNER(g:cloud-dwh)

PY3_LIBRARY()

PEERDIR(
    nirvana/valhalla/src
)

PY_SRCS(__init__.py)

RESOURCE(
    cloud/dwh/nirvana/vh/workflows/cdm/yt/datalens/dm_datalens_funnel_with_promo/resources/query.sql cdm/yt/datalens/dm_datalens_funnel_with_promo/resources/query.sql
    cloud/dwh/nirvana/vh/workflows/cdm/yt/datalens/dm_datalens_funnel_with_promo/resources/parameters.yaml cdm/yt/datalens/dm_datalens_funnel_with_promo/resources/parameters.yaml
)

END()
