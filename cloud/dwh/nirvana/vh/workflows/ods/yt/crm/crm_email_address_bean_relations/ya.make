OWNER(g:cloud-dwh)

PY3_LIBRARY()

PEERDIR(
    nirvana/valhalla/src
)

PY_SRCS(__init__.py)

RESOURCE(
    cloud/dwh/nirvana/vh/workflows/ods/yt/crm/crm_email_address_bean_relations/resources/query.sql ods/yt/crm/crm_email_address_bean_relations/resources/query.sql
    cloud/dwh/nirvana/vh/workflows/ods/yt/crm/crm_email_address_bean_relations/resources/parameters.yaml ods/yt/crm/crm_email_address_bean_relations/resources/parameters.yaml
)

END()
