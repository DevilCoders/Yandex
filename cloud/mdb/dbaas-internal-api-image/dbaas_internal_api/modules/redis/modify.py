"""
DBaaS Internal API Redis cluster modification
"""
from datetime import timedelta
from typing import List, Optional, Tuple

from marshmallow import Schema

from ...core.exceptions import NoChangesError, ParseConfigError
from ...core.types import Operation
from ...utils import metadb, validation
from ...utils.backups import get_backup_schedule
from ...utils.cluster.update import is_downscale, resources_diff, update_cluster_labels, update_cluster_name
from ...utils.feature_flags import ensure_no_feature_flag
from ...utils.host import get_hosts
from ...utils.infra import get_resources_strict
from ...utils.metadata import ModifyClusterMetadata
from ...utils.modify import update_cluster_resources, update_field
from ...utils.network import validate_security_groups
from ...utils.operation_creator import create_finished_operation, create_operation
from ...utils.register import DbaasOperation, Resource, register_request_handler
from ...utils.types import (
    ClusterInfo,
    ClusterStatus,
    LabelsDict,
    MaintenanceWindowDict,
    OperationParameters,
    def_disk_type_id_parser,
)
from ...utils.validation import check_cluster_not_in_status
from ...utils.version import ensure_version_allowed, is_version_deprecated
from .constants import MY_CLUSTER_TYPE
from .hosts import get_masters
from .pillar import RedisPillar
from .traits import RedisOperations, RedisTasks
from .types import RedisConfigSpec
from .utils import (
    check_if_tls_available,
    get_cluster_nodes_validate_func,
    get_cluster_version,
    validate_client_output_buffer_limit,
    validate_disk_size,
    validate_password_change,
    validate_public_ip,
    validate_version_change,
)


@register_request_handler(MY_CLUSTER_TYPE, Resource.CLUSTER, DbaasOperation.MODIFY)
def modify_redis_cluster(
    cluster_obj: dict,
    _schema: Schema,
    config_spec: dict = None,
    labels: LabelsDict = None,
    description: str = None,
    name: Optional[str] = None,
    maintenance_window: Optional[MaintenanceWindowDict] = None,
    security_group_ids: List[str] = None,
    deletion_protection: Optional[bool] = None,
    persistence_mode: Optional[str] = None,
    **_
) -> Operation:
    """
    Modifies Redis cluster.
    """
    ensure_no_feature_flag('MDB_REDIS_DISABLE_LEGACY_CLUSTER_CRUD_API')
    changes = False
    restart = _schema.context.get('restart', False)
    restart_only_sentinel = False
    task_args = {}  # type: dict
    task_type = RedisTasks.modify
    time_limit = None

    if config_spec is not None or persistence_mode is not None:
        task_type, changes, restart_needed, restart_only_sentinel, task_args, time_limit = _modify_redis_cluster(
            cluster_obj=cluster_obj,
            _schema=_schema,
            config_spec=config_spec,
            persistence_mode=persistence_mode,
        )
        restart |= restart_needed

    if update_cluster_name(cluster_obj, name):
        task_args.update({'include-metadata': True})
        changes = restart_only_sentinel = True

    have_metadb_changes = False
    if update_cluster_labels(cluster_obj, labels):
        have_metadb_changes = True

    if security_group_ids is not None:
        sg_ids = set(security_group_ids)
        if set(cluster_obj.get('user_sgroup_ids', [])) != sg_ids:
            validate_security_groups(sg_ids, cluster_obj['network_id'])
            task_args['security_group_ids'] = list(sg_ids)
            changes = True

    if changes:
        check_cluster_not_in_status(ClusterInfo.make(cluster_obj), ClusterStatus.stopped)
        update_task_args_on_restart(cluster_obj, task_args, restart, restart_only_sentinel)
        return create_operation(
            task_type=task_type,
            time_limit=time_limit,
            operation_type=RedisOperations.modify,
            metadata=ModifyClusterMetadata(),
            cid=cluster_obj['cid'],
            task_args=task_args,
        )

    if have_metadb_changes:
        return create_finished_operation(
            operation_type=RedisOperations.modify,
            metadata=ModifyClusterMetadata(),
            cid=cluster_obj['cid'],
        )

    raise NoChangesError()


def update_task_args_on_restart(cluster_obj: dict, task_args: dict, restart: bool, restart_only_sentinel: bool):
    if restart:
        task_args.update({'restart': True})
        task_args.update({'masters': get_masters(cluster_obj)})
        return
    if restart_only_sentinel:
        task_args.update({'restart_only_sentinel': True})


def _modify_conf(
    states: list,
    task_type: RedisTasks,
    config_spec: dict,
    cid: str,
    pillar: RedisPillar,
    cluster_obj: dict,
) -> Tuple[list, RedisTasks, bool, bool]:
    try:
        conf_obj = RedisConfigSpec(config_spec, version_required=False)
    except (ValueError, KeyError) as err:
        raise ParseConfigError(err)

    version = get_cluster_version(cid)

    password_changed = validate_password_change(conf_obj, pillar.password)
    version_changed = False
    if conf_obj.version and conf_obj.version != version:
        version_state, version_changed = process_version_changes(pillar, conf_obj, cluster_obj)
        if version_state.changes:
            task_type = RedisTasks.upgrade
        states.append(version_state)

    check_if_tls_available(pillar.tls_enabled, conf_obj.version or version)
    hosts = list(get_hosts(cid).values())
    validate_public_ip(hosts, pillar.tls_enabled)

    new_flavor = None
    disk_type_id = None
    flavor_changed = False
    if conf_obj.has_resources():
        current = get_resources_strict(cid)
        resources = conf_obj.get_resources(disk_type_id_parser=def_disk_type_id_parser)

        disk_type_id = resources.disk_type_id or current.disk_type_id
        resource_preset_id = resources.resource_preset_id or current.resource_preset_id
        disk_size = resources.disk_size or current.disk_size
        flavor = validation.get_flavor_by_name(resource_preset_id)
        validate_disk_size(flavor, disk_size)
        validate_func = get_cluster_nodes_validate_func(flavor, disk_type_id, pillar.is_cluster_enabled())
        state, flavor_changed = update_cluster_resources(cid, MY_CLUSTER_TYPE, resources, validate_func=validate_func)
        if flavor_changed:
            new_flavor = flavor
            is_memory_down = is_downscale(resources_diff(hosts, new_flavor)[0], ['memory_guarantee'])
            if is_memory_down:
                state.task_args.update({'memory_downscaled': True})
        states.append(state)

    states.append(
        handle_config_changes(
            cluster_obj=cluster_obj,
            pillar=pillar,
            conf_obj=conf_obj,
            new_flavor=new_flavor,
        )
    )

    validate_client_output_buffer_limit(conf_obj)

    config = conf_obj.get_config()
    databases_num = config.get('databases')
    databases_num_changed = databases_num is not None and databases_num != pillar.databases
    restart_needed = version_changed or flavor_changed or databases_num_changed
    restart_only_sentinel = password_changed

    return states, task_type, restart_needed, restart_only_sentinel


def _modify_redis_cluster(
    cluster_obj: dict,
    _schema: Schema,
    config_spec: Optional[dict],
    persistence_mode: Optional[str],
) -> Tuple[RedisTasks, bool, bool, bool, dict, timedelta]:
    """
    Modifies Redis cluster.
    Returns (task_type, changes, restart_needed, restart_only_sentinel, task_args, time_limit).
    """
    states: list = []
    changes = False
    restart_needed = False
    restart_only_sentinel = False
    task_args = {}
    task_type = RedisTasks.modify
    time_limit = timedelta(hours=1)
    cid = cluster_obj['cid']
    pillar = RedisPillar(cluster_obj['value'])

    if config_spec:
        states, task_type, restart_needed, restart_only_sentinel = _modify_conf(
            states,
            task_type,
            config_spec,
            cid,
            pillar,
            cluster_obj,
        )

    if persistence_mode:
        persistence_changed = pillar.process_persistence_mode(persistence_mode)
        changes = changes or persistence_changed

    for state in states:
        task_args.update(state.task_args)
        time_limit += state.time_limit
        changes = changes or state.changes

    if changes:
        metadb.update_cluster_pillar(cid, pillar)

    return task_type, changes, restart_needed, restart_only_sentinel, task_args, time_limit


def process_version_changes(
    pillar: RedisPillar, conf_obj: RedisConfigSpec, cluster_obj: dict
) -> Tuple[OperationParameters, bool]:
    """
    Process major version changes
    """
    cid = cluster_obj['cid']
    env = cluster_obj['env']
    new_version = conf_obj.version
    cur_version = get_cluster_version(cid)

    if not is_version_deprecated(MY_CLUSTER_TYPE, cur_version):
        ensure_version_allowed(MY_CLUSTER_TYPE, new_version)
    validate_version_change(cur_version=cur_version, conf_obj=conf_obj)

    metadb.set_default_versions(
        cid=cid,
        subcid=None,
        shard_id=None,
        env=env,
        major_version=new_version,
        edition=cur_version.edition,
        ctype=MY_CLUSTER_TYPE,
    )

    return (
        OperationParameters(
            True,
            {
                'upgrade': True,
                'upgrade_from': cur_version.to_num(),
                'upgrade_to': new_version.to_num(),
            },
            timedelta(hours=1),
        ),
        True,
    )


def handle_config_changes(
    cluster_obj: dict,
    pillar: RedisPillar,
    conf_obj: RedisConfigSpec,
    new_flavor: Optional[dict] = None,
) -> OperationParameters:
    """
    Check if there are config changes to handle.
    """
    changes = False
    task_args = {}
    target_config = pillar.config
    target_config.update(conf_obj.get_config())

    if pillar.config != target_config:
        changes = True
        pillar.update_config(target_config)

    if new_flavor:
        changes = True
        pillar.set_flavor_dependent_options(new_flavor)

    changes |= pillar.update_client_buffers(
        conf_obj.client_output_limit_buffer_normal, conf_obj.client_output_limit_buffer_pubsub
    )

    for field in ('backup_start', 'access'):
        changed = update_field(field, conf_obj, pillar)
        changes |= changed
        if changed:
            task_args.update({'include-metadata': True})

    if conf_obj.backup_start:
        metadb.add_backup_schedule(cluster_obj['cid'], get_backup_schedule(MY_CLUSTER_TYPE, conf_obj.backup_start))

    if changes:
        metadb.update_cluster_pillar(cluster_obj['cid'], pillar)

    return OperationParameters(changes=changes, task_args=task_args, time_limit=timedelta(hours=0))
