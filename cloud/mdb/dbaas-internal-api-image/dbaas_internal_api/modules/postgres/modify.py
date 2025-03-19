# -*- coding: utf-8 -*-
"""
DBaaS Internal API postgresql cluster modify
"""
from datetime import timedelta
from typing import Optional, Tuple, List

from marshmallow import Schema

from . import utils
from ...core.exceptions import DbaasClientError, NoChangesError, ParseConfigError
from ...core.types import Operation
from ...utils import infra, metadb, modify
from ...utils.backups import get_backup_schedule
from ...utils.metadata import ModifyClusterMetadata
from ...utils.network import validate_security_groups
from ...utils.modify import update_field
from ...utils.operation_creator import create_operation
from ...utils.register import DbaasOperation, Resource, register_request_handler
from ...utils.types import ClusterInfo, ClusterStatus, MaintenanceWindowDict
from ...utils.validation import check_cluster_not_in_status
from ...utils.version import Version, ensure_version_allowed
from .constants import (
    MY_CLUSTER_TYPE,
    RESTART_DEFAULTS,
    DEFAULT_MAX_REPLICATION_SLOTS,
    DEFAULT_MAX_WAL_SENDERS,
    DEFAULT_MAX_LOGICAL_REPLICATION_WORKERS,
    DEFAULT_MAX_WORKER_PROCESSES,
)
from .pillar import PostgresqlClusterPillar, get_cluster_pillar
from .traits import PostgresqlOperations, PostgresqlTasks
from .types import PostgresqlConfigSpec
from .utils import get_max_connections_limit, get_cluster_version
from .validation import (
    ALLOWED_SHARED_PRELOAD_LIBS,
    UPGRADE_TASKS,
    validate_version_upgrade,
    validate_databases,
)


def _modify_postgresql_cluster(  # pylint: disable=too-many-locals,too-many-statements,too-many-branches
    cluster_obj: dict, cluster_pillar: PostgresqlClusterPillar, _schema: Schema, config_spec: dict
) -> Tuple[bool, PostgresqlTasks, dict, timedelta]:
    """
    Creates postgresql cluster. Returns task for worker
    """

    cid = cluster_obj['cid']
    env = cluster_obj['env']

    version = get_cluster_version(cid)
    task = PostgresqlTasks.modify
    changes = False
    instance_type_changed = False
    task_args = {}  # type: dict
    time_limit = timedelta(hours=1)

    try:
        # Load and validate version
        conf_obj = PostgresqlConfigSpec(config_spec, version_required=False, fill_default=False)
    except ValueError as err:
        raise ParseConfigError(err)

    # Version upgrade
    new_version = str(conf_obj.version)
    if conf_obj.version and version.to_string() != new_version:
        ensure_version_allowed(MY_CLUSTER_TYPE, Version.load(new_version))
        validate_version_upgrade(version.to_string(), new_version)
        metadb.set_default_versions(
            cid=cid,
            subcid=None,
            shard_id=None,
            env=env,
            major_version=new_version.replace('-1c', ''),
            edition=version.edition,
            ctype=MY_CLUSTER_TYPE,
        )

        task = UPGRADE_TASKS[new_version]
        changes = True

    config_options = conf_obj.get_config()

    old_resources_obj = infra.get_resources(cid)
    old_flavor = None
    if conf_obj.has_resources():
        resources_obj = conf_obj.get_resources()
        state, instance_type_changed = modify.update_cluster_resources(cid, MY_CLUSTER_TYPE, resources_obj)
        changes = state.changes or changes
        task_args.update(state.task_args)
        time_limit += state.time_limit
        old_flavor = metadb.get_flavor_by_name(old_resources_obj.resource_preset_id)
    else:
        resources_obj = old_resources_obj

    flavor = metadb.get_flavor_by_name(
        resources_obj.resource_preset_id
        if resources_obj.resource_preset_id is not None
        else old_resources_obj.resource_preset_id
    )
    restart = False

    # Validate postgresql config

    user_specs = cluster_pillar.pgusers.get_users()
    config_pillar = cluster_pillar.config.get_config()

    shared_preload_libs_default = config_pillar.get('user_shared_preload_libraries')
    shared_preload_libs = config_options.get('user_shared_preload_libraries', shared_preload_libs_default) or []

    validate_databases(cluster_pillar.databases.get_databases())

    # we won’t immediately ask for the size of the disk, maybe we dont need it in current request
    disk_size = None

    if config_options:
        for config in config_options:
            for lib in ALLOWED_SHARED_PRELOAD_LIBS:
                if lib in config and lib not in shared_preload_libs:
                    raise DbaasClientError(
                        '{} cannot be changed without using {} in shared_preload_libraries'.format(config, lib)
                    )
        if config_options.get('wal_level') is not None:
            raise DbaasClientError('wal_level is deprecated and no longer supported')

        disk_size = (
            resources_obj.disk_size if resources_obj.disk_size is not None else infra.get_resources(cid).disk_size
        )
        utils.validate_postgres_options(config_options, user_specs, flavor, disk_size)
        changes = True
        if _schema.context.get('restart'):
            restart = True
            # reverse_order == restart master -> restart replica

            reverse_order_inited = False
            order_reason = ''
            if not task_args.get('reverse_order') and instance_type_changed:
                order_reason = 'Upscale'
                reverse_order_inited = True
                task_args['reverse_order'] = False
            if task_args.get('reverse_order') and instance_type_changed:
                order_reason = 'Downscale'
                reverse_order_inited = True
                task_args['reverse_order'] = True

            # These options on replicas should be greater or equal then on master
            options = [
                'max_connections',
                'max_locks_per_transaction',
                'max_worker_processes',
                'max_prepared_transactions',
                'max_wal_senders',
            ]

            default_data = metadb.get_cluster_type_pillar(MY_CLUSTER_TYPE)['data']
            for option in options:
                if config_options.get(option) is not None:

                    new_value = config_options[option]
                    if option == 'max_connections':
                        old_value = cluster_pillar.config.max_connections or get_max_connections_limit(flavor)
                    else:
                        old_value = config_pillar.get(option) or default_data['config'][option]

                    if new_value < old_value and not task_args.get('reverse_order') and reverse_order_inited:
                        raise DbaasClientError('{0} cannot be mixed with reducing {1}'.format(order_reason, option))

                    if new_value > old_value and task_args.get('reverse_order') and reverse_order_inited:
                        raise DbaasClientError('{0} cannot be mixed with increasing {1}'.format(order_reason, option))

                    if order_reason == '':
                        if new_value < old_value:
                            order_reason = 'Reducing {0}'.format(option)
                            task_args['reverse_order'] = True
                            reverse_order_inited = True
                        if new_value > old_value:
                            order_reason = 'Increasing {0}'.format(option)
                            task_args['reverse_order'] = False
                            reverse_order_inited = True

            max_replication_slots_conf = config_options.get('max_replication_slots')
            if max_replication_slots_conf is not None:
                changes = True
                # Get effective value at first from cid then from cluster_type pillar
                max_replication_effective = config_pillar.get(
                    'max_replication_slots',
                    default_data['config'].get('max_replication_slots', DEFAULT_MAX_REPLICATION_SLOTS),
                )
                if max_replication_effective == max_replication_slots_conf:
                    changes = False
                if max_replication_effective > max_replication_slots_conf:
                    raise DbaasClientError('max_replication_slots cannot be decreased')

            max_wal_senders_conf = config_options.get('max_wal_senders')
            if max_wal_senders_conf is not None:
                changes = True
                max_wal_senders_effective = config_pillar.get(
                    'max_wal_senders', default_data['config'].get('max_wal_senders', DEFAULT_MAX_WAL_SENDERS)
                )
                if max_wal_senders_effective == max_wal_senders_conf:
                    changes = False

            max_logical_replication_workers_conf = config_options.get('max_logical_replication_workers')
            if max_logical_replication_workers_conf is not None:
                changes = True
                max_logical_replication_workers_effective = config_pillar.get(
                    'max_logical_replication_workers',
                    default_data['config'].get(
                        'max_logical_replication_workers', DEFAULT_MAX_LOGICAL_REPLICATION_WORKERS
                    ),
                )
                max_worker_processes_effective = config_pillar.get(
                    'max_worker_processes',
                    default_data['config'].get('max_worker_processes', DEFAULT_MAX_WORKER_PROCESSES),
                )
                if max_logical_replication_workers_effective == max_logical_replication_workers_conf:
                    changes = False
                if max_logical_replication_workers_conf >= max_worker_processes_effective:
                    raise DbaasClientError(
                        'max_logical_replication_workers cannot be greater or equal max_worker_processes'
                    )

        cluster_pillar.config.merge_config(config_options)

    if instance_type_changed:
        config_pillar = cluster_pillar.config.get_config()

        if disk_size is None:
            # find out the disk size in the cluster if not already
            # this is done so as not to access the metadb 2 times during the current request so it takes a long time
            disk_size = (
                resources_obj.disk_size if resources_obj.disk_size is not None else infra.get_resources(cid).disk_size
            )

        utils.validate_postgres_options(config_pillar, user_specs, flavor, disk_size)

        if any([default_option not in config_pillar for default_option in RESTART_DEFAULTS]):
            restart = True

    # Make sure the new settings fit the old flavor (MDB-17417)
    if old_flavor and task_args.get('reverse_order'):
        utils.validate_postgres_options(config_pillar, user_specs, old_flavor, old_resources_obj.disk_size)

    # Validate pooler config
    if 'pooler_config' in config_spec:
        changes = True
        cluster_pillar.config.merge_config(config_spec['pooler_config'])

    if 'perf_diag' in config_spec:
        perf_diag_changes = False
        perf_diag_config = cluster_pillar.perf_diag.get()
        for field in ('enable', 'pgsa_sample_period', 'pgss_sample_period'):
            perf_diag_changes = perf_diag_changes | (
                field in config_spec['perf_diag']
                and (config_spec['perf_diag'][field] != perf_diag_config.get(field, None))
            )
            if field == 'enable' and perf_diag_changes:
                restart = True
        if perf_diag_changes:
            changes = True
            cluster_pillar.perf_diag.update(config_spec['perf_diag'])

    if 'autofailover' in config_spec:
        changes = True
        cluster_pillar.pgsync.set_autofailover(config_spec['autofailover'])

    if 'sox_audit' in config_spec and cluster_pillar.sox_audit != config_spec['sox_audit']:
        if config_spec['sox_audit'] is False:
            raise DbaasClientError('Cannot turn off sox audit.')
        for sox_audit_user in ('reader', 'writer'):
            cluster_pillar.pgusers.assert_user_not_exists(sox_audit_user)
        changes = True
        task_args['sox-changed'] = True
        cluster_pillar.sox_audit = config_spec['sox_audit']

    for field in ('access', 'backup_start'):
        changed = update_field(field, conf_obj, cluster_pillar)
        if changed:
            task_args['include-metadata'] = True
        changes |= changed

    try:
        # Load and validate version
        cluster_backup_opts = PostgresqlConfigSpec(
            cluster_obj['backup_schedule'], version_required=False, config_spec_required=False
        )
    except ValueError as err:
        raise ParseConfigError(err)

    for old_opt, opt in [
        (cluster_backup_opts.backup_start, conf_obj.backup_start),
        (cluster_backup_opts.retain_period, conf_obj.retain_period),
    ]:
        changes |= opt is not None and old_opt != opt

    if conf_obj.backup_start or conf_obj.retain_period:
        metadb.add_backup_schedule(
            cid,
            get_backup_schedule(
                MY_CLUSTER_TYPE,
                conf_obj.backup_start if conf_obj.backup_start else cluster_backup_opts.backup_start,
                conf_obj.retain_period if conf_obj.retain_period else cluster_backup_opts.retain_period,
                cluster_backup_opts.use_backup_service,
                cluster_backup_opts.max_incremental_steps,
            ),
        )

    if changes:
        metadb.update_cluster_pillar(cid, cluster_pillar)

    return (
        changes,
        task,
        {
            'restart': restart,
            **task_args,
        },
        time_limit,
    )


@register_request_handler(MY_CLUSTER_TYPE, Resource.CLUSTER, DbaasOperation.MODIFY)
def modify_postgresql_cluster(
    cluster_obj: dict,
    _schema: Schema,
    config_spec: dict = None,
    labels: Optional[dict] = None,
    description: Optional[str] = None,
    name: Optional[str] = None,
    maintenance_window: Optional[MaintenanceWindowDict] = None,
    security_group_ids: List[str] = None,
    deletion_protection: Optional[bool] = None,
    **_
) -> Operation:
    """
    Modifies postgresql cluster. Returns task for worker
    """
    cluster_pillar = get_cluster_pillar(cluster_obj)
    task_type = PostgresqlTasks.modify
    changes = False
    task_args = {}  # type: dict
    time_limit = None

    if config_spec is not None:
        changes, task_type, task_args, time_limit = _modify_postgresql_cluster(
            cluster_obj=cluster_obj,
            cluster_pillar=cluster_pillar,
            config_spec=config_spec,
            _schema=_schema,
        )

    task_args['zk_hosts'] = cluster_pillar.pgsync.get_zk_hosts_as_str()

    if security_group_ids is not None:
        sg_ids = set(security_group_ids)
        if set(cluster_obj.get('user_sgroup_ids', [])) != sg_ids:
            validate_security_groups(sg_ids, cluster_obj['network_id'])
            task_args['security_group_ids'] = list(sg_ids)
            changes = True

    if changes:
        check_cluster_not_in_status(ClusterInfo.make(cluster_obj), ClusterStatus.stopped)
        return create_operation(
            task_type=task_type,
            operation_type=PostgresqlOperations.modify,
            metadata=ModifyClusterMetadata(),
            cid=cluster_obj['cid'],
            time_limit=time_limit,
            task_args=task_args,
        )

    raise NoChangesError()
