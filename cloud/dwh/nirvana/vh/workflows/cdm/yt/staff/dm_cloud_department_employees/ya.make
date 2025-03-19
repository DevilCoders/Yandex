OWNER(g:cloud-dwh)

PY3_LIBRARY()

PEERDIR(
    nirvana/valhalla/src
)

PY_SRCS(__init__.py)

RESOURCE(
    cloud/dwh/nirvana/vh/workflows/cdm/yt/staff/dm_cloud_department_employees/resources/query.sql cdm/yt/staff/dm_cloud_department_employees/resources/query.sql
    cloud/dwh/nirvana/vh/workflows/cdm/yt/staff/dm_cloud_department_employees/resources/parameters.yaml cdm/yt/staff/dm_cloud_department_employees/resources/parameters.yaml
)

END()
