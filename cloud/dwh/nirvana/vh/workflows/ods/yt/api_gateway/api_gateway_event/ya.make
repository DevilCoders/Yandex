OWNER(g:cloud-dwh)

PY3_LIBRARY()

PEERDIR(
    nirvana/valhalla/src
)

PY_SRCS(__init__.py)

RESOURCE(
    cloud/dwh/nirvana/vh/workflows/ods/yt/api_gateway/api_gateway_event/resources/query.sql ods/yt/api_gateway/api_gateway_event/resources/query.sql
    cloud/dwh/nirvana/vh/workflows/ods/yt/api_gateway/api_gateway_event/resources/parameters.yaml ods/yt/api_gateway/api_gateway_event/resources/parameters.yaml
)

END()
