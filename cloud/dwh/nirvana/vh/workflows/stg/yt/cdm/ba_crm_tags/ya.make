OWNER(g:cloud-dwh)

PY3_LIBRARY()

PEERDIR(
    nirvana/valhalla/src
)

PY_SRCS(__init__.py)

RESOURCE(
    cloud/dwh/nirvana/vh/workflows/stg/yt/cdm/ba_crm_tags/resources/query.sql stg/yt/cdm/ba_crm_tags/resources/query.sql
    cloud/dwh/nirvana/vh/workflows/stg/yt/cdm/ba_crm_tags/resources/parameters.yaml stg/yt/cdm/ba_crm_tags/resources/parameters.yaml
    cloud/dwh/nirvana/vh/workflows/stg/yt/cdm/ba_crm_tags/resources/check_billing_accounts.sql stg/yt/cdm/ba_crm_tags/resources/check_billing_accounts.sql
    cloud/dwh/nirvana/vh/workflows/stg/yt/cdm/ba_crm_tags/resources/check_duplicates_by_date.sql stg/yt/cdm/ba_crm_tags/resources/check_duplicates_by_date.sql
    cloud/dwh/nirvana/vh/workflows/stg/yt/cdm/ba_crm_tags/resources/check_not_null_current_segment.sql stg/yt/cdm/ba_crm_tags/resources/check_not_null_current_segment.sql
)

END()
