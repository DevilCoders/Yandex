# -*- coding: utf-8 -*-
"""
DBaaS Internal API MetaDB query helpers
"""

from flask import request
import opentracing

from ..query import BaseQueryCache

TRACE_QUERY_ARGS = {
    # Resources
    'cloud_id': 'cloud.id',
    'cloud_ext_id': 'cloud.ext_id',
    'folder_id': 'folder.id',
    'folder_ext_id': 'folder.ext_id',
    # Cluster info
    'cluster_id': 'cluster.id',
    'cid': 'cluster.id',
    'pillar_cid': 'cluster.id',
    'ctype': 'cluster.type',
    'cluster_type': 'cluster.type',
    'role': 'cluster.role',
    'visibility': 'cluster.visibility',
    'rev': 'cluster.revision',
    'env': 'cluster.environment',
    'deletion_protection': 'cluster.protection.deletion',
    # Sub cluster
    'subcid': 'subcluster.id',
    'pillar_subcid': 'subcluster.id',
    # Shard id
    'shard_id': 'shard.id',
    'pillar_shard_id': 'shard.id',
    # Hosts
    'fqdn': 'host.fqdn',
    'pillar_fqdn': 'host.fqdn',
    'geo': 'host.geo',
    'subnet_id': 'host.subnet.id',
    'assign_public_ip': 'host.public_ip.assign',
    'disk_type_id': 'host.disk.type.id',
    'vtype_id': 'host.vtype.id',
    # Worker
    'task_id': 'worker.task.id',
    'task_type': 'worker.task.type',
    'operation_id': 'worker.operation.id',
    'operation_type': 'worker.operation.type',
    'hidden': 'worker.task.hidden',
    # Cluster resource presets
    'flavor': 'cluster.resource_preset.flavour.id',
    'flavor_id': 'cluster.resource_preset.flavour.id',
    'flavor_type': 'cluster.resource_preset.flavour.type',
    'generation': 'cluster.resource_preset.generation',
    # Maintenance window
    'day': 'maintenance.window.day',
    'hour': 'maintenance.window.hour',
    'config_id': 'maintenance.config_id',
    'plan_ts': 'maintenance.plan_ts',
    # Alerts
    'ag_id': 'alert.group.id',
    'alert_group_id': 'alert.group.id',
    'monitoring_folder_id': 'alert.folder.id',
    'template_id': 'alert.template.id',
    'disabled': 'alert.disabled',
    # Hadoop
    'job_id': 'hadoop.job.id',
    # Paging
    'page_token_id': 'paging.token.id',
    'page_token_subcid': 'paging.token.id',
    'page_token_name': 'paging.token.id',
    'page_token_job_id': 'paging.token.id',
    'page_token_create_ts': 'paging.token.create_ts',
    'limit': 'paging.limit',
    # Infrastructure
    'master': 'db.master',
    'fetch': 'db.fetch',
    'x_request_id': 'request.id',
    'idempotence_id': 'idempotence.id',
}


class QueryCache(BaseQueryCache):
    """
    PostgreSQL-specific query conf
    """

    def __init__(self, *args, **kwargs):
        BaseQueryCache.__init__(self, *args, **kwargs)
        self.reload(__file__, __name__)

    def query(self, cur, name, query_args, fetch=True):
        """
        Execute query (optionally fetching result)
        """
        if name not in self.queries:
            raise RuntimeError('%s not found in query_conf' % name)

        try:
            mogrified = cur.mogrify(self.queries[name], query_args)
        except KeyError as exc:
            raise RuntimeError(f'"{name}" query require argument: {exc}') from exc
        self.logger.debug(
            mogrified.decode('utf-8'),
            extra={
                'request': request,
                'metadb_host': cur.connection.get_dsn_parameters().get(
                    'host',
                    'localhost',
                ),
            },
        )

        with opentracing.global_tracer().start_active_span('DB Query', finish_on_close=True) as scope:
            for key, value in query_args.items():
                if key in TRACE_QUERY_ARGS:
                    scope.span.set_tag(TRACE_QUERY_ARGS[key], value)
            scope.span.set_tag('db.type', 'sql')
            scope.span.set_tag('db.statement.name', name)
            scope.span.set_tag(
                'db.instance',
                cur.connection.get_dsn_parameters().get(
                    'host',
                    'localhost',
                ),
            )

            cur.execute(mogrified)

        return cur.fetchall() if fetch else None
