OWNER(g:cloud-dwh)

PY3_LIBRARY()

PEERDIR(
    nirvana/valhalla/src
)

PY_SRCS(__init__.py)

RESOURCE(
    cloud/dwh/nirvana/vh/workflows/ods/yt/statbox/currency_rates/resources/query.sql ods/yt/statbox/currency_rates/resources/query.sql
    cloud/dwh/nirvana/vh/workflows/ods/yt/statbox/currency_rates/resources/parameters.yaml ods/yt/statbox/currency_rates/resources/parameters.yaml
)

END()
