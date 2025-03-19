PY3_PROGRAM()
OWNER(the-nans)

PEERDIR(
    contrib/python/certifi
    contrib/python/requests
    contrib/python/prettytable
    contrib/python/PyYAML
    contrib/python/PyJWT
    yql/library/python
    cloud/bitbucket/common-api/yandex/cloud/api
    cloud/bitbucket/common-api/yandex/cloud/api/tools
    cloud/bitbucket/private-api/yandex/cloud/priv
    cloud/bitbucket/private-api/yandex/cloud/priv/billing/v1
    cloud/bitbucket/private-api/yandex/cloud/priv/console/v1
    cloud/bitbucket/private-api/yandex/cloud/priv/compute/v1/admin
    cloud/bitbucket/private-api/yandex/cloud/priv/resourcemanager/v1
    cloud/bitbucket/private-api/yandex/cloud/priv/resourcemanager/v1/console
    cloud/bitbucket/private-api/yandex/cloud/priv/iam/v1/ts
    cloud/bitbucket/private-api/yandex/cloud/priv/iam/v1
    cloud/bitbucket/private-api/yandex/cloud/priv/iam/v1/backoffice
    cloud/bitbucket/private-api/yandex/cloud/priv/iam/v1/console
    cloud/bitbucket/private-api/yandex/cloud/priv/vpc/v1
    cloud/bitbucket/private-api/yandex/cloud/priv/k8s/v1
    cloud/bitbucket/private-api/yandex/cloud/priv/k8s/v1/inner
    cloud/bitbucket/private-api/yandex/cloud/priv/mdb/elasticsearch/v1
    cloud/bitbucket/private-api/yandex/cloud/priv/mdb/greenplum/v1
    cloud/bitbucket/private-api/yandex/cloud/priv/mdb/sqlserver/v1
    cloud/bitbucket/private-api/yandex/cloud/priv/vpc/v1/inner
    cloud/bitbucket/private-api/yandex/cloud/priv/reference
    cloud/bitbucket/private-api/yandex/cloud/priv/operation
    cloud/bitbucket/private-api/yandex/cloud/priv/access
    cloud/bitbucket/private-api/yandex/cloud/priv/quota
    cloud/bitbucket/private-api/yandex/cloud/priv/mdb/kafka/v1
    cloud/bitbucket/private-api/yandex/cloud/priv/mdb/elasticsearch/v1
    cloud/bitbucket/private-api/yandex/cloud/priv/mdb/greenplum/v1
    cloud/bitbucket/private-api/yandex/cloud/priv/mdb/sqlserver/v1




)
PY_SRCS(
    TOP_LEVEL
    MAIN saint.py
    assets.py
    cloudfacade.py
    endpoints.py
    grpc_gw.py
    helpers.py
    parser.py
    printers.py
    profiles.py
    saint_cloud_components/st_auth.py
    saint_cloud_components/st_billing.py
    saint_cloud_components/st_cloud.py
    saint_cloud_components/st_disk.py
    saint_cloud_components/st_folder.py
    saint_cloud_components/st_instance.py
    saint_cloud_components/st_k8s.py
    saint_cloud_components/st_mdb.py
    saint_cloud_components/st_network.py
    saint_cloud_components/st_operation.py
    saint_cloud_components/st_s3.py
    saint_cloud_components/st_user.py
    saint_cloud_components/st_compute_host.py

)
END()
