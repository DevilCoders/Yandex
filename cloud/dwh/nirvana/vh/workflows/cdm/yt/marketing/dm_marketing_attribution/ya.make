OWNER(g:cloud-dwh)

PY3_LIBRARY()

PEERDIR(
    nirvana/valhalla/src
)

PY_SRCS(__init__.py)

RESOURCE(
    cloud/dwh/nirvana/vh/workflows/cdm/yt/marketing/dm_marketing_attribution/resources/query.sql cdm/yt/marketing/dm_marketing_attribution/resources/query.sql
    cloud/dwh/nirvana/vh/workflows/cdm/yt/marketing/dm_marketing_attribution/resources/parameters.yaml cdm/yt/marketing/dm_marketing_attribution/resources/parameters.yaml
    cloud/dwh/nirvana/vh/workflows/cdm/yt/marketing/dm_marketing_attribution/resources/utils/attribution_functions.sql cdm/yt/marketing/dm_marketing_attribution/resources/utils/attribution_functions.sql
    cloud/dwh/nirvana/vh/workflows/cdm/yt/marketing/dm_marketing_attribution/udf/attribution_functions.py cdm/yt/marketing/dm_marketing_attribution/resources/udf/attribution_functions.py
)

END()
