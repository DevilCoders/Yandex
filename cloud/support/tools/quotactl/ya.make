PY3_PROGRAM(qctl)

OWNER(apereshein)

PEERDIR(
    contrib/python/requests
    contrib/python/prettytable
    contrib/python/PyYAML
    contrib/python/PyJWT
    contrib/python/decorator
    contrib/python/prompt-toolkit
    cloud/bitbucket/common-api/yandex/cloud/api/tools
    cloud/bitbucket/private-api/yandex/cloud/priv
    cloud/bitbucket/private-api/yandex/cloud/priv/serverless/functions/v1
    cloud/bitbucket/private-api/yandex/cloud/priv/serverless/triggers/v1
    cloud/bitbucket/private-api/yandex/cloud/priv/microcosm/instancegroup/v1
    cloud/bitbucket/private-api/yandex/cloud/priv/k8s/v1
    cloud/bitbucket/private-api/yandex/cloud/priv/containerregistry/v1
    cloud/bitbucket/private-api/yandex/cloud/priv/compute/v1
    cloud/bitbucket/private-api/yandex/cloud/priv/vpc/v1
    cloud/bitbucket/private-api/yandex/cloud/priv/monitoring/v2
    cloud/bitbucket/private-api/yandex/cloud/priv/iot/devices/v1
    cloud/bitbucket/private-api/yandex/cloud/priv/loadbalancer/v1
    cloud/bitbucket/private-api/yandex/cloud/priv/iam/v1
    cloud/bitbucket/private-api/yandex/cloud/priv/iam/v1/ts
    cloud/bitbucket/private-api/yandex/cloud/priv/resourcemanager/v1/transitional
    cloud/bitbucket/private-api/yandex/cloud/priv/ydb/v1
    cloud/bitbucket/private-api/yandex/cloud/priv/platform/alb/v1
    cloud/bitbucket/private-api/yandex/cloud/priv/dns/v1
)


PY_SRCS(
    TOP_LEVEL
    MAIN main.py

    cli/__init__.py
    quota/__init__.py
    quota/base.py
    quota/constants.py
    quota/error.py
    quota/metric.py
    quota/subject.py
    quota/version.py
    quota/utils/__init__.py
    quota/utils/autocomplete.py
    quota/utils/helpers.py
    quota/utils/request.py
    quota/utils/response.py
    quota/utils/validators.py
    quota/services/__init__.py
    quota/services/billing.py
    quota/services/compute.py
    quota/services/container_registry.py
    quota/services/iot.py
    quota/services/instance_group.py
    quota/services/kms.py
    quota/services/kubernetes.py
    quota/services/load_balancer.py
    quota/services/monitoring.py
    quota/services/mdb.py
    quota/services/object_storage.py
    quota/services/quota_calculator.py
    quota/services/resource_manager.py
    quota/services/serverless.py
    quota/services/vpc.py
    quota/services/alb.py
    quota/services/ydb.py
    quota/services/dns.py
)

END()
