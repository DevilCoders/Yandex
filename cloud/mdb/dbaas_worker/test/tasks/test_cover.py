"""
Task test coverage check
"""

import ast
from pathlib import Path

from cloud.mdb.dbaas_worker.internal.tasks.utils import EXECUTORS
from yatest.common import source_path

SERVICE_ALERT_SKIP = {
    'clickhouse_add_zookeeper',
    'clickhouse_cluster_create_backup',
    'clickhouse_cluster_delete_metadata',
    'clickhouse_cluster_maintenance',
    'clickhouse_metadata_update',
    'clickhouse_cluster_move',
    'clickhouse_cluster_offline_resetup',
    'clickhouse_cluster_online_resetup',
    'clickhouse_cluster_purge',
    'clickhouse_cluster_restore',
    'clickhouse_cluster_start',
    'clickhouse_cluster_stop',
    'clickhouse_cluster_update_tls_certs',
    'clickhouse_cluster_upgrade',
    'clickhouse_cluster_wait_backup_service',
    'clickhouse_database_create',
    'clickhouse_database_delete',
    'clickhouse_dictionary_create',
    'clickhouse_dictionary_delete',
    'clickhouse_format_schema_create',
    'clickhouse_format_schema_delete',
    'clickhouse_format_schema_modify',
    'clickhouse_model_create',
    'clickhouse_model_delete',
    'clickhouse_model_modify',
    'clickhouse_remove_zookeeper',
    'clickhouse_user_create',
    'clickhouse_user_delete',
    'clickhouse_user_modify',
    'clickhouse_host_create',
    'clickhouse_host_modify',
    'clickhouse_zookeeper_host_create',
    'clickhouse_zookeeper_host_create',
    'clickhouse_shard_create',
    'elasticsearch_cluster_create_backup',
    'elasticsearch_cluster_delete_metadata',
    'elasticsearch_cluster_offline_resetup',
    'elasticsearch_cluster_online_resetup',
    'elasticsearch_cluster_purge',
    'elasticsearch_cluster_start',
    'elasticsearch_cluster_stop',
    'elasticsearch_cluster_update_tls_certs',
    'elasticsearch_cluster_upgrade',
    'elasticsearch_cluster_maintenance',
    'elasticsearch_metadata_update',
    'elasticsearch_host_create',
    'elasticsearch_user_create',
    'elasticsearch_user_delete',
    'elasticsearch_user_modify',
    'greenplum_host_create',
    'greenplum_cluster_delete_metadata',
    'greenplum_cluster_offline_resetup',
    'greenplum_cluster_online_resetup',
    'greenplum_cluster_purge',
    'greenplum_cluster_start',
    'greenplum_cluster_stop',
    'greenplum_cluster_update_tls_certs',
    'greenplum_cluster_maintenance',
    'greenplum_metadata_update',
    'greenplum_cluster_restore',
    'greenplum_cluster_start_segment_failover',
    'hadoop_cluster_modify',
    'hadoop_cluster_start',
    'hadoop_cluster_stop',
    'hadoop_cluster_create',
    'hadoop_cluster_delete',
    'hadoop_subcluster_create',
    'hadoop_subcluster_delete',
    'hadoop_subcluster_modify',
    'metastore_cluster_modify',
    'metastore_cluster_start',
    'metastore_cluster_stop',
    'metastore_cluster_create',
    'metastore_cluster_delete',
    'metastore_cluster_delete_metadata',
    'kafka_cluster_delete_metadata',
    'kafka_cluster_maintenance',
    'kafka_metadata_update',
    'kafka_cluster_move',
    'kafka_cluster_offline_resetup',
    'kafka_cluster_online_resetup',
    'kafka_cluster_start',
    'kafka_cluster_stop',
    'kafka_cluster_update_tls_certs',
    'kafka_cluster_upgrade',
    'kafka_connector_create',
    'kafka_connector_delete',
    'kafka_connector_pause',
    'kafka_connector_resume',
    'kafka_connector_update',
    'kafka_topic_create',
    'kafka_topic_delete',
    'kafka_topic_modify',
    'kafka_user_create',
    'kafka_user_delete',
    'kafka_user_modify',
    'kafka_host_create',
    'kafka_host_delete',
    'kafka_zookeeper_host_create',
    'mongodb_backup_delete',
    'mongodb_cluster_create_backup',
    'mongodb_cluster_delete_metadata',
    'mongodb_cluster_enable_sharding',
    'mongodb_cluster_maintenance',
    'mongodb_metadata_update',
    'mongodb_cluster_move',
    'mongodb_cluster_offline_resetup',
    'mongodb_cluster_online_resetup',
    'mongodb_cluster_purge',
    'mongodb_cluster_resetup_hosts',
    'mongodb_host_create',
    'mongodb_cluster_restore',
    'mongodb_cluster_start',
    'mongodb_cluster_stepdown_hosts',
    'mongodb_cluster_restart_hosts',
    'mongodb_cluster_stop',
    'mongodb_cluster_update_tls_certs',
    'mongodb_cluster_upgrade',
    'mongodb_cluster_wait_backup_service',
    'mongodb_database_create',
    'mongodb_database_delete',
    'mongodb_shard_create',
    'mongodb_user_create',
    'mongodb_user_delete',
    'mongodb_user_modify',
    'mysql_cluster_create_backup',
    'mysql_cluster_delete_metadata',
    'mysql_cluster_maintenance',
    'mysql_metadata_update',
    'mysql_cluster_move',
    'mysql_cluster_offline_resetup',
    'mysql_cluster_online_resetup',
    'mysql_cluster_purge',
    'mysql_cluster_restore',
    'mysql_cluster_start',
    'mysql_cluster_start_failover',
    'mysql_cluster_stop',
    'mysql_cluster_update_tls_certs',
    'mysql_cluster_upgrade_80',
    'mysql_cluster_wait_backup_service',
    'mysql_database_create',
    'mysql_database_delete',
    'mysql_host_create',
    'mysql_host_modify',
    'mysql_user_create',
    'mysql_user_delete',
    'mysql_user_modify',
    'noop',
    'postgresql_backup_delete',
    'postgresql_cluster_create_backup',
    'postgresql_cluster_delete_metadata',
    'postgresql_cluster_maintenance',
    'postgresql_cluster_fast_maintenance',
    'postgresql_metadata_update',
    'postgresql_cluster_move',
    'postgresql_cluster_offline_resetup',
    'postgresql_cluster_online_resetup',
    'postgresql_cluster_purge',
    'postgresql_cluster_restore',
    'postgresql_cluster_start',
    'postgresql_cluster_start_failover',
    'postgresql_cluster_stop',
    'postgresql_cluster_update_tls_certs',
    'postgresql_cluster_upgrade_11',
    'postgresql_cluster_upgrade_11_1c',
    'postgresql_cluster_upgrade_12',
    'postgresql_cluster_upgrade_13',
    'postgresql_cluster_upgrade_14',
    'postgresql_cluster_wait_backup_service',
    'postgresql_database_create',
    'postgresql_database_delete',
    'postgresql_database_modify',
    'postgresql_user_create',
    'postgresql_user_delete',
    'postgresql_user_modify',
    'postgresql_host_create',
    'postgresql_host_modify',
    'redis_cluster_create_backup',
    'redis_cluster_delete_metadata',
    'redis_cluster_maintenance',
    'redis_metadata_update',
    'redis_cluster_move',
    'redis_cluster_offline_resetup',
    'redis_cluster_online_resetup',
    'redis_cluster_purge',
    'redis_cluster_rebalance',
    'redis_cluster_restore',
    'redis_cluster_start',
    'redis_cluster_start_failover',
    'redis_cluster_stop',
    'redis_cluster_upgrade',
    'redis_shard_host_create',
    'redis_shard_host_delete',
    'redis_host_create',
    'redis_shard_create',
    'sqlserver_cluster_create_backup',
    'sqlserver_cluster_delete_metadata',
    'sqlserver_metadata_update',
    'sqlserver_cluster_purge',
    'sqlserver_cluster_restore',
    'sqlserver_cluster_start',
    'sqlserver_cluster_start_failover',
    'sqlserver_cluster_stop',
    'sqlserver_cluster_update_tls_certs',
    'sqlserver_database_create',
    'sqlserver_database_delete',
    'sqlserver_database_modify',
    'sqlserver_database_restore',
    'sqlserver_host_modify',
    'sqlserver_user_create',
    'sqlserver_user_delete',
    'sqlserver_user_modify',
    'sqlserver_database_backup_export',
    'sqlserver_database_backup_import',
    'opensearch_cluster_create_backup',
    'opensearch_cluster_delete_metadata',
    'opensearch_cluster_maintenance',
    'opensearch_cluster_offline_resetup',
    'opensearch_cluster_online_resetup',
    'opensearch_cluster_purge',
    'opensearch_cluster_start',
    'opensearch_cluster_stop',
    'opensearch_cluster_upgrade',
    'opensearch_metadata_update',
    'opensearch_host_create',
    'opensearch_user_create',
    'opensearch_user_delete',
    'opensearch_user_modify',
}

INTERRUPT_CONSISTENCY_SKIP = {
    'noop',
}

MLOCK_USAGE_SKIP = {
    'noop',
    'hadoop_cluster_create',
    'hadoop_cluster_delete',
    'hadoop_cluster_modify',
    'hadoop_cluster_start',
    'hadoop_cluster_stop',
    'hadoop_subcluster_create',
    'hadoop_subcluster_delete',
    'hadoop_subcluster_modify',
    'clickhouse_cluster_purge',
    'mongodb_cluster_purge',
    'mysql_cluster_purge',
    'elasticsearch_cluster_purge',
    'greenplum_cluster_purge',
    'postgresql_cluster_purge',
    'redis_cluster_purge',
    'sqlserver_cluster_purge',
    'clickhouse_cluster_delete_metadata',
    'elasticsearch_cluster_delete_metadata',
    'greenplum_cluster_delete_metadata',
    'kafka_cluster_delete_metadata',
    'mongodb_cluster_delete_metadata',
    'mysql_cluster_delete_metadata',
    'postgresql_cluster_delete_metadata',
    'redis_cluster_delete_metadata',
    'sqlserver_cluster_delete_metadata',
    'metastore_cluster_modify',
    'metastore_cluster_start',
    'metastore_cluster_stop',
    'metastore_cluster_create',
    'metastore_cluster_delete',
    'metastore_cluster_delete_metadata',
    'opensearch_cluster_purge',
    'opensearch_cluster_delete_metadata',
}


def get_checked():
    """
    Get sets of tasks with checks applied
    """
    ret = {
        'interrupt': set(),
        'mlock': set(),
        'service_alerts': set(),
    }
    for path in Path(source_path('cloud/mdb/dbaas_worker/test/tasks')).rglob('*.py'):
        with open(path) as inp:
            tree = ast.parse(inp.read())

        for expression in tree.body:
            if isinstance(expression, ast.FunctionDef) and expression.name.startswith('test_'):
                for inner_expression in expression.body:
                    if (
                        hasattr(inner_expression, 'value')
                        and isinstance(inner_expression.value, ast.Call)
                        and hasattr(inner_expression.value.func, 'id')
                    ):
                        if inner_expression.value.func.id == 'check_task_interrupt_consistency':
                            ret['interrupt'].add(inner_expression.value.args[1].s)
                        elif inner_expression.value.func.id == 'check_mlock_usage':
                            ret['mlock'].add(inner_expression.value.args[1].s)
                        elif inner_expression.value.func.id == 'check_alerts_synchronised':
                            ret['service_alerts'].add(inner_expression.value.args[1].s)

    return ret


def test_check_cover():
    """
    Check that all tasks are covered by checks
    """
    covered = get_checked()
    missing = {
        'interrupt': [],
        'mlock': [],
        'service_alerts': [],
    }
    for executor_name in EXECUTORS:
        if executor_name not in covered['interrupt'] and executor_name not in INTERRUPT_CONSISTENCY_SKIP:
            missing['interrupt'].append(executor_name)
        if executor_name not in covered['mlock'] and executor_name not in MLOCK_USAGE_SKIP:
            missing['mlock'].append(executor_name)
        if executor_name not in covered['service_alerts'] and executor_name not in SERVICE_ALERT_SKIP:
            missing['service_alerts'].append(executor_name)
    msgs = []
    for check_type in missing:
        if missing[check_type]:
            msgs.append(f'{check_type}: {", ".join(sorted(missing[check_type]))}.')
    assert not msgs, 'Some tasks are not covered by checks.\n' + '\n'.join(msgs)
