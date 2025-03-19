PY3_LIBRARY()

STYLE_PYTHON()

OWNER(g:mdb)

FORK_TESTS()

PEERDIR(
    cloud/bitbucket/private-api/yandex/cloud/priv/iam/v1
    cloud/bitbucket/private-api/yandex/cloud/priv/reference
    cloud/bitbucket/private-api/yandex/cloud/priv/iam/v1/awscompatibility
    cloud/bitbucket/private-api/yandex/cloud/priv/dataproc/manager/v1
    cloud/bitbucket/private-api/yandex/cloud/priv/dns/v1
    cloud/bitbucket/private-api/yandex/cloud/priv/microcosm/instancegroup/v1
    cloud/bitbucket/private-api/third_party/envoy/api/envoy/config/core/v3
    cloud/bitbucket/private-api/third_party/envoy/api/envoy/config/endpoint/v3
    cloud/bitbucket/private-api/third_party/envoy/api/envoy/service/discovery/v3
    cloud/bitbucket/private-api/third_party/envoy/api/envoy/service/endpoint/v3
    cloud/mdb/dbaas_python_common
    cloud/mdb/internal/python/compute/iam/jwt
    cloud/mdb/internal/python/compute/iam/operations
    cloud/mdb/internal/python/compute/dns
    cloud/mdb/internal/python/compute/vpc
    cloud/mdb/internal/python/compute/instances
    cloud/mdb/internal/python/compute/disks
    cloud/mdb/internal/python/compute/images
    cloud/mdb/internal/python/compute/operations
    cloud/mdb/internal/python/compute/disk_placement_groups
    cloud/mdb/internal/python/compute/host_groups
    cloud/mdb/internal/python/compute/host_type
    cloud/mdb/internal/python/compute/placement_groups
    cloud/mdb/internal/python/vpc
    cloud/mdb/internal/python/managed_kubernetes
    cloud/mdb/internal/python/loadbalancer
    cloud/mdb/internal/python/lockbox
    cloud/mdb/internal/python/managed_postgresql
    cloud/mdb/mlock/api
    library/python/resource
    library/python/svn_version
    contrib/python/botocore
    contrib/python/boto3
    contrib/python/dateutil
    contrib/python/dnspython
    contrib/python/kazoo
    cloud/mdb/internal/python/logs
    contrib/python/paramiko
    contrib/python/PGPy
    contrib/python/PyNaCl
    contrib/python/pydantic
    contrib/python/PyYAML
    contrib/python/psycopg2
    contrib/python/raven
    contrib/python/setproctitle
    contrib/python/opentracing
    contrib/python/service-identity
    contrib/python/pyOpenSSL
    contrib/python/kubernetes
    contrib/python/lxml
)

RESOURCE(
    sqls/alert_delete_by_id.sql sqls/alert_delete_by_id.sql
    sqls/set_alert_active.sql sqls/set_alert_active.sql
    sqls/get_alerts_by_cid.sql sqls/get_alerts_by_cid.sql
    sqls/acquire_task.sql sqls/acquire_task.sql
    sqls/increment_failed_acquire_count.sql sqls/increment_failed_acquire_count.sql
    sqls/get_task.sql sqls/get_task.sql
    sqls/add_cluster_target_pillar.sql sqls/add_cluster_target_pillar.sql
    sqls/add_sgroups.sql sqls/add_sgroups.sql
    sqls/complete_cluster_change.sql sqls/complete_cluster_change.sql
    sqls/delete_sgroups.sql sqls/delete_sgroups.sql
    sqls/delete_task_target_pillar.sql sqls/delete_task_target_pillar.sql
    sqls/enable_billing.sql sqls/enable_billing.sql
    sqls/disable_billing.sql sqls/disable_billing.sql
    sqls/finish_task.sql sqls/finish_task.sql
    sqls/generic_resolve.sql sqls/generic_resolve.sql
    sqls/get_host_info.sql sqls/get_host_info.sql
    sqls/get_host_by_vtype_id.sql sqls/get_host_by_vtype_id.sql
    sqls/get_instance_group.sql sqls/get_instance_group.sql
    sqls/get_kubernetes_node_group.sql sqls/get_kubernetes_node_group.sql
    sqls/get_pillar.sql sqls/get_pillar.sql
    sqls/get_major_version.sql sqls/get_major_version.sql
    sqls/get_reject_rev.sql sqls/get_reject_rev.sql
    sqls/get_running_tasks.sql sqls/get_running_tasks.sql
    sqls/get_sgroups_info.sql sqls/get_sgroups_info.sql
    sqls/get_subcluster_by_instance_group_id.sql sqls/get_subcluster_by_instance_group_id.sql
    sqls/get_subcluster_by_kubernetes_node_group_id.sql sqls/get_subcluster_by_kubernetes_node_group_id.sql
    sqls/get_version.sql sqls/get_version.sql
    sqls/lock_cluster.sql sqls/lock_cluster.sql
    sqls/poll_cancelled.sql sqls/poll_cancelled.sql
    sqls/poll_delayed.sql sqls/poll_delayed.sql
    sqls/poll_new.sql sqls/poll_new.sql
    sqls/reject_task.sql sqls/reject_task.sql
    sqls/release_task.sql sqls/release_task.sql
    sqls/reschedule_task.sql sqls/reschedule_task.sql
    sqls/restart_task.sql sqls/restart_task.sql
    sqls/update_host_vtype_id.sql sqls/update_host_vtype_id.sql
    sqls/update_host_subnet_id.sql sqls/update_host_subnet_id.sql
    sqls/update_instance_group.sql sqls/update_instance_group.sql
    sqls/update_kubernetes_node_group.sql sqls/update_kubernetes_node_group.sql
    sqls/update_sgroup_hash.sql sqls/update_sgroup_hash.sql
    sqls/update_task.sql sqls/update_task.sql
    sqls/upsert_pillar.sql sqls/upsert_pillar.sql
    sqls/get_backup_info.sql sqls/get_backup_info.sql
    sqls/mark_cluster_backups_deleted.sql sqls/mark_cluster_backups_deleted.sql
    sqls/get_disks_info.sql sqls/get_disks_info.sql
    sqls/update_disk.sql sqls/update_disk.sql
    sqls/update_disk_placement_group.sql sqls/update_disk_placement_group.sql
    sqls/schedule_initial_backup.sql sqls/schedule_initial_backup.sql
    sqls/update_placement_group.sql sqls/update_placement_group.sql
    sqls/get_cluster_project_id.sql sqls/get_cluster_project_id.sql

    resources/metastore/secret.yaml resources/metastore/secret.yaml
    resources/metastore/external-secret.yaml resources/metastore/external-secret.yaml
    resources/metastore/metastore-db-init-job.yaml resources/metastore/metastore-db-init-job.yaml
    resources/metastore/external-secret-store.yaml resources/metastore/external-secret-store.yaml
    resources/metastore/metastore-server.yaml resources/metastore/metastore-server.yaml
    resources/metastore/metastore-service.yaml resources/metastore/metastore-service.yaml
    resources/metastore/metastore-site-configmap.yaml resources/metastore/metastore-site-configmap.yaml
)

ALL_PY_SRCS(
    RECURSIVE
    NAMESPACE
    cloud.mdb.dbaas_worker.internal
)

END()
