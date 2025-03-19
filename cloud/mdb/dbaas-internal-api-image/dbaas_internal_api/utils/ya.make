OWNER(g:mdb)

PY3_LIBRARY()

STYLE_PYTHON()

PY_SRCS(
    NAMESPACE dbaas_internal_api.utils
    __init__.py
    alert_group.py
    backup_id.py
    backups.py
    billing.py
    cluster_secrets.py
    compute.py
    compute_billing.py
    compute_quota.py
    config.py
    constants.py
    e2e.py
    events.py
    feature_flags.py
    filters_parser.py
    helpers.py
    host.py
    iam_jwt.py
    idempotence.py
    identity.py
    infra.py
    instance_group.py
    io_limit.py
    logging_service.py
    logging_read_service.py
    logs.py
    maintenance.py
    maintenance_helpers.py
    metadata.py
    metadb.py
    modify.py
    network.py
    operation_creator.py
    operations.py
    pagination.py
    pillar.py
    quota.py
    register.py
    renderers.py
    request_context.py
    resource_manager.py
    restore_validation.py
    s3.py
    search_renders.py
    tasks_version.py
    time.py
    traits.py
    types.py
    validation.py
    version.py
    worker.py
    zk.py
)

PEERDIR(
    contrib/python/Flask
    cloud/bitbucket/private-api/yandex/cloud/priv/compute/v1
    cloud/bitbucket/private-api/yandex/cloud/priv/microcosm/instancegroup/v1
    cloud/bitbucket/private-api/yandex/cloud/priv/logging/v1
    cloud/bitbucket/private-api/yandex/cloud/priv/quota
    cloud/mdb/dbaas-internal-api-image/dbaas_internal_api/utils/apispec
    cloud/mdb/dbaas-internal-api-image/dbaas_internal_api/utils/iam_token
    cloud/mdb/dbaas-internal-api-image/dbaas_internal_api/utils/cluster
    cloud/mdb/dbaas-internal-api-image/dbaas_internal_api/utils/dataproc_joblog
    cloud/mdb/dbaas-internal-api-image/dbaas_internal_api/utils/dataproc_manager
    cloud/mdb/internal/python/compute/clouds
    cloud/mdb/internal/python/compute/folders
    cloud/mdb/internal/python/compute/iam/jwt
    cloud/mdb/internal/python/racktables
    cloud/mdb/internal/python/yandex_team/abc
)

RESOURCE(
    filters_grammar.lark utils/filters_grammar.lark
)

END()

RECURSE(
    iam_token
    apispec
    cluster
    dataproc_manager
)
