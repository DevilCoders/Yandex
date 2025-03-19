# -*- coding: utf-8 -*-
"""
DBaaS Internal API MySQL cluster modification
"""
from datetime import timedelta
from .hosts import get_ha_host_pillars
from .info import get_cluster_config_set
from typing import List, Dict, Any, Optional, Tuple

from flask_restful import abort
from marshmallow import Schema

from ...core.exceptions import NoChangesError, ParseConfigError
from ...core.types import Operation
from ...utils import infra, metadb, modify, validation
from ...utils.backups import get_backup_schedule
from ...utils.helpers import merge_dict
from ...utils.metadata import ModifyClusterMetadata
from ...utils.network import validate_security_groups
from ...utils.modify import update_field
from ...utils.operation_creator import create_operation
from ...utils.register import DbaasOperation, Resource, register_request_handler
from ...utils.types import (
    ENV_PROD,
    VTYPE_PORTO,
    ClusterInfo,
    ClusterStatus,
    LabelsDict,
    OperationParameters,
    MaintenanceWindowDict,
)
from ...utils.validation import check_cluster_not_in_status
from ...utils.version import Version, ensure_version_allowed
from ..mysql.validation import UPGRADE_TASK_TO_OPERATION, UPGRADE_TASKS, validate_version_upgrade, fix_pillar_on_upgrade
from .constants import MY_CLUSTER_TYPE, RESTART_DEFAULTS
from .pillar import MySQLPillar, get_cluster_pillar
from .traits import MySQLOperations, MySQLRoles, MySQLTasks
from .types import MysqlConfigSpec
from .utils import (
    validate_config_changes,
    validate_need_restart,
    validate_options,
    get_cluster_version,
    populate_idm_system_users_if_need,
)


def get_instance_type_obj(conf_obj: MysqlConfigSpec, cluster_id: str) -> dict:
    """
    Instance type -- resolves current or intended (what user requested)
    """
    if conf_obj.has_resources():
        resources_obj = conf_obj.get_resources()
        if resources_obj.resource_preset_id is not None:
            return validation.get_flavor_by_name(resources_obj.resource_preset_id)
    resources_obj = infra.get_resources(cluster_id, MySQLRoles.mysql)
    return metadb.get_flavor_by_name(resources_obj.resource_preset_id)


def handle_config_changes(
    schema_obj: Schema,
    pillar: MySQLPillar,
    cluster_obj: dict,
    conf_obj: MysqlConfigSpec,
    instance_type_changed: bool,
) -> OperationParameters:
    """Facilitate config changes"""

    # Handle restarts due to config or instance type changes
    restart = False
    changes = False
    time_limit = timedelta(hours=0)
    task_args = {}
    flavor = get_instance_type_obj(conf_obj, cluster_obj['cid'])

    sox_audit = conf_obj.key(
        'sox_audit',
        flavor['vtype'].lower() == VTYPE_PORTO.lower() and cluster_obj['env'].lower() == ENV_PROD.lower(),
    )

    validation.validate_sox_flag(flavor['vtype'], cluster_obj['env'], sox_audit)

    if schema_obj.context.get('restart'):
        restart = True

    for field in ('backup_start', 'access', 'perf_diag'):
        changed = update_field(field, conf_obj, pillar)
        changes |= changed
        if changed:
            task_args['include-metadata'] = True

    if conf_obj.sox_audit != pillar.sox_audit:
        changes = True
        pillar.sox_audit = conf_obj.sox_audit
        populate_idm_system_users_if_need(pillar, conf_obj.sox_audit, Version.load(pillar.mysql_version))
        task_args['include-metadata'] = True

    try:
        # Load and validate version
        cluster_backup_opts = MysqlConfigSpec(cluster_obj['backup_schedule'], version_required=False, fill_default=True)
    except ValueError as err:
        raise ParseConfigError(err)

    if conf_obj.backup_start or conf_obj.retain_period:
        # these fields are stored in a single JSON, so we should send all of them at once:
        # merge default_schedule, cluster_obj's schedule (with it's own defaults) and conf_obj's schedule:
        metadb.add_backup_schedule(
            cluster_obj['cid'],
            get_backup_schedule(
                MY_CLUSTER_TYPE,
                backup_window_start=conf_obj.backup_start
                if conf_obj.backup_start
                else cluster_backup_opts.backup_start,
                backup_retain_period=conf_obj.retain_period
                if conf_obj.retain_period
                else cluster_backup_opts.retain_period,
                use_backup_service=cluster_backup_opts.use_backup_service
                if cluster_backup_opts.use_backup_service
                else False,
            ),
        )

    old_config = pillar.config.get_config()

    if instance_type_changed:
        # Only restart if there are no instance-type dependant
        # settings in the pillar
        if any([default_option not in old_config for default_option in RESTART_DEFAULTS]):
            restart = True
    # There is no config provided, bail out.

    new_config = conf_obj.get_config()

    config_options = None
    if new_config:
        config_options = old_config
        merge_dict(config_options, new_config)
    elif instance_type_changed:
        # there is no new options in config but we still need to validate old resource dependent options
        config_options = old_config

    if config_options:
        version = pillar.mysql_version
        if version is None:
            raise RuntimeError('Version not specified in spec')
        hosts = get_ha_host_pillars(cluster_obj['cid'])
        config_set = get_cluster_config_set(cluster=cluster_obj, pillar=pillar, flavor=flavor)

        validate_config_changes(config_set.effective, config_options)
        validate_options(config_set.effective, config_options, version, flavor, len(hosts))
        if validate_need_restart(config_set.effective, new_config):
            restart = True

        pillar.config.merge_config(config_options)
        changes = True

    if restart:
        task_args['restart'] = True
    return OperationParameters(changes=changes, task_args=task_args, time_limit=time_limit)


def _modify_mysql_cluster(
    cluster_obj: dict,
    pillar: MySQLPillar,
    _schema: Schema,
    config_spec: Optional[dict],
    security_group_ids: List[str] = None,
) -> Tuple[bool, MySQLTasks, dict, Optional[timedelta]]:
    """
    Modifies MySQL cluster.
    Updates pillar and returns changes flag and task_args.

    Logic is separated into three steps:
    1. Determine if there are version changes.
    2. Determine if there are resource changes.
    3. Determine if there are config changes.
    """

    time_limit: Optional[timedelta] = None
    changes = False
    task = MySQLTasks.modify
    task_args: Dict[Any, Any] = {}

    zk_hosts = pillar.zk_hosts.copy()
    task_args['zk_hosts'] = zk_hosts

    if security_group_ids is not None:
        sg_ids = set(security_group_ids)
        if set(cluster_obj.get('user_sgroup_ids', [])) != sg_ids:
            validate_security_groups(sg_ids, cluster_obj['network_id'])
            task_args['security_group_ids'] = list(sg_ids)
            changes = True

    if config_spec is None:
        return changes, task, task_args, time_limit

    instance_type_changed = False
    states = []  # type: List[OperationParameters]
    time_limit = timedelta(hours=1)

    try:
        # Load and validate version
        conf_obj = MysqlConfigSpec(config_spec, version_required=False, fill_default=False)
    except (ValueError, KeyError) as err:
        # `raise` to explicitly tell linters that processing stops here.
        raise abort(422, message='error parsing configSpec: {0}'.format(err))

    cur_version = get_cluster_version(cluster_obj['cid'], pillar)

    # Version upgrade
    new_version = str(conf_obj.version)
    if conf_obj.version and str(cur_version) != new_version:
        ensure_version_allowed(MY_CLUSTER_TYPE, Version.load(new_version))
        validate_version_upgrade(str(cur_version), new_version)
        metadb.set_default_versions(
            cid=cluster_obj['cid'],
            subcid=None,
            shard_id=None,
            env=cluster_obj['env'],
            major_version=new_version,
            edition=cur_version.edition,
            ctype=MY_CLUSTER_TYPE,
        )
        pillar.mysql_version = new_version
        fix_pillar_on_upgrade(cur_version, conf_obj.version, pillar)
        states.append(OperationParameters(changes=True, task_args={}, time_limit=timedelta(hours=0)))
        task = UPGRADE_TASKS[new_version]

    if conf_obj.has_resources():
        state, instance_type_changed = modify.update_cluster_resources(
            cluster_obj['cid'], MY_CLUSTER_TYPE, conf_obj.get_resources()
        )
        time_limit += state.time_limit
        states.append(state)

    states.append(
        handle_config_changes(
            _schema,
            pillar,
            cluster_obj,
            conf_obj,
            instance_type_changed,
        )
    )

    # Pick up task arguments
    for state in states:
        task_args.update(state.task_args)

    changes |= any((x.changes for x in states))
    if changes:
        metadb.update_cluster_pillar(cluster_obj['cid'], pillar)

    return changes, task, task_args, time_limit


def _calc_task_type(changes: bool, task: MySQLTasks) -> Tuple[bool, MySQLTasks, MySQLOperations]:
    operation = MySQLOperations.modify
    res_task = task

    if changes:
        upgrade_op = UPGRADE_TASK_TO_OPERATION.get(task)
        if upgrade_op:
            operation = upgrade_op
    else:
        return False, res_task, operation

    return True, res_task, operation


@register_request_handler(MY_CLUSTER_TYPE, Resource.CLUSTER, DbaasOperation.MODIFY)
def modify_mysql_cluster(
    cluster_obj: dict,
    _schema: Schema,
    config_spec: dict = None,
    labels: LabelsDict = None,
    description: str = None,
    name: Optional[str] = None,
    maintenance_window: Optional[MaintenanceWindowDict] = None,
    security_group_ids: List[str] = None,
    deletion_protection: Optional[bool] = None,
    **_
) -> Operation:
    """
    Modifies MySQL cluster. Returns operation.
    """

    changes = False

    # pillar will be updated inside _modify_mysql_cluster
    pillar = get_cluster_pillar(cluster_obj)

    changes, task, task_args, time_limit = _modify_mysql_cluster(
        pillar=pillar,
        cluster_obj=cluster_obj,
        _schema=_schema,
        config_spec=config_spec,
        security_group_ids=security_group_ids,
    )

    need_run_operation, task, operation = _calc_task_type(
        changes=changes,
        task=task,
    )

    if need_run_operation:
        check_cluster_not_in_status(ClusterInfo.make(cluster_obj), ClusterStatus.stopped)
        return create_operation(
            cid=cluster_obj['cid'],
            task_type=task,
            operation_type=operation,
            metadata=ModifyClusterMetadata(),
            time_limit=time_limit,
            task_args=task_args,
        )

    raise NoChangesError()
