# -*- coding: utf-8 -*-
"""
DBaaS Internal API MongoDB cluster modification
"""
from datetime import timedelta
from typing import Any, Optional, Set, Tuple, List

from marshmallow import Schema

from . import utils
from ...core.exceptions import DbaasClientError, NoChangesError, ParseConfigError, PreconditionFailedError
from ...core.types import Operation
from ...utils import infra, metadb, modify, validation
from ...utils.backups import get_backup_schedule
from ...utils.helpers import get_value_by_path
from ...utils.metadata import ModifyClusterMetadata
from ...utils.modify import update_field
from ...utils.network import validate_security_groups
from ...utils.operation_creator import create_operation
from ...utils.register import DbaasOperation, Resource, register_request_handler
from ...utils.types import ClusterInfo, ClusterStatus, LabelsDict, OperationParameters, MaintenanceWindowDict
from ...utils.validation import check_cluster_not_in_status
from ...utils.version import ensure_version_allowed, Version
from .constants import MY_CLUSTER_TYPE, RESTART_DEFAULTS
from .pillar import MongoDBPillar, get_cluster_pillar
from .traits import MongoDBOperations, MongoDBRoles, MongoDBTasks
from .types import MongodbConfigSpec
from .utils import get_cluster_version, validate_fcv, validate_version_change


def get_instance_type_obj(conf_obj: MongodbConfigSpec, service: str, cluster_id: str) -> dict:
    """
    Instance type -- resolves current or intended (what user requested)
    """
    if conf_obj.has_resources(service):
        resources_obj = conf_obj.get_resources(service)
        if resources_obj.resource_preset_id is not None:
            return validation.get_flavor_by_name(resources_obj.resource_preset_id)
    resources_obj = infra.get_resources(cluster_id)
    return metadb.get_flavor_by_name(resources_obj.resource_preset_id)


def handle_config_changes(
    _: Schema,
    service: str,
    pillar: MongoDBPillar,
    cluster_obj: dict,
    conf_obj: MongodbConfigSpec,
    instance_type_changed: bool,
) -> OperationParameters:
    """Facilitate config changes"""

    # We can not apply mongodb changes without restart for now,
    # so run service restart if its config is found in update spec
    has_config = conf_obj.has_config(role=service)
    if service == 'mongoinfra':
        has_config = conf_obj.has_config(role=service, key='config_mongos') or conf_obj.has_config(
            role=service, key='config_mongocfg'
        )
    restart = has_config

    if instance_type_changed:
        # Only restart if there are no instance-type dependant
        # settings in the pillar
        if any(
            [get_value_by_path(pillar.config, default_option.split('.')) is None for default_option in RESTART_DEFAULTS]
        ):
            restart = True
    # There is no config provided, bail out.
    if not has_config:
        return OperationParameters(changes=False, task_args={'restart': restart}, time_limit=timedelta(hours=0))

    # Current or new instance type to validate settings
    instance_type_obj = get_instance_type_obj(conf_obj, service, cluster_obj['cid'])
    configs = {}
    if service == 'mongoinfra':
        if conf_obj.has_config(role=service, key='config_mongos'):
            configs['mongos'] = conf_obj.get_config(service, 'config_mongos')
        if conf_obj.has_config(role=service, key='config_mongocfg'):
            configs['mongocfg'] = conf_obj.get_config(service, 'config_mongocfg')
    else:
        configs[service] = conf_obj.get_config(service)

    for srv, service_conf in configs.items():
        utils.validate_options(
            {
                srv: service_conf,
            },
            conf_obj.version,
            instance_type_obj,
        )

        pillar.update_config({srv: service_conf})

    metadb.update_cluster_pillar(cluster_obj['cid'], pillar)
    return OperationParameters(changes=True, task_args={'restart': restart}, time_limit=timedelta(hours=0))


def handle_service_spec_changes(
    conf_obj: MongodbConfigSpec, _schema: Schema, pillar: MongoDBPillar, cluster_obj: dict
) -> OperationParameters:
    """
    Facilitate changes in service specs
    """

    changes = False
    restart_services = []
    reverse_deploy_order: Set[Any] = set()
    task_args = {}
    time_limit = timedelta(hours=1)

    # Little preparation because of MongoInfra host has mongos and mongocfg configs
    if (conf_obj.has_config('mongoinfra', 'config_mongos') and conf_obj.has_config('mongos')) or (
        conf_obj.has_config('mongoinfra', 'config_mongocfg') and conf_obj.has_config('mongocfg')
    ):
        raise DbaasClientError(
            "You should not provide both mongoinfra.config_mongos and mongos.config or mongoinfra.config_mongocfg and mongocfg.config"
        )

    for entry in MongoDBRoles:
        service = entry.name
        role = entry.value
        instance_type_changed = False
        changes_requested = conf_obj.has_resources(service) or conf_obj.has_config(service)
        if role == MongoDBRoles.mongoinfra and not changes_requested:
            changes_requested = conf_obj.has_config(service, 'config_mongos') or conf_obj.has_config(
                service, 'config_mongocfg'
            )
        if changes_requested and not pillar.sharding_enabled and entry != MongoDBRoles.mongod:
            raise PreconditionFailedError('Sharding must be enabled in order to change {0} settings'.format(service))

        if conf_obj.has_resources(service):
            state, instance_type_changed = modify.update_cluster_resources(
                cluster_obj['cid'], MY_CLUSTER_TYPE, conf_obj.get_resources(service), role
            )
            changes = changes or state.changes
            if state.changes:
                reverse_deploy_order.add(state.task_args.get('reverse_order'))
            time_limit += state.time_limit
            # MDB-11495 - In case of host restart, we could need to wait
            # for host resetup to be completed, so set timeout to at least 1 day
            time_limit = max(time_limit, timedelta(days=1))

        service_modify = handle_config_changes(
            _schema,
            service,
            pillar,
            cluster_obj,
            conf_obj,
            instance_type_changed,
        )
        changes = changes or service_modify.changes
        if service_modify.task_args.get('restart'):
            restart_services.append(service)

    if len(reverse_deploy_order) > 1:
        raise DbaasClientError('Upscale of one type of hosts cannot be mixed with downscale of another type of hosts')

    if restart_services:
        if 'mongos' in restart_services or 'mongocfg' in restart_services:
            restart_services.append('mongoinfra')
            # Restart MongoInfra in case of any of it's services need to be restarted
            # For example, if we changed config of MongoCFG, we need to restart MongoInfra as well
        task_args.update({'restart_services': restart_services})

    if reverse_deploy_order:
        task_args.update({'reverse_order': reverse_deploy_order.pop()})

    return OperationParameters(changes=changes, task_args=task_args, time_limit=time_limit)


def handle_version_change(conf_obj: MongodbConfigSpec, pillar: MongoDBPillar, cluster_obj: dict) -> OperationParameters:
    """
    Facilitate version change
    """

    assert conf_obj.version is not None

    if not conf_obj.has_version_only():
        raise DbaasClientError('Version change cannot be mixed with other modifications')

    cid = cluster_obj['cid']
    env = cluster_obj['env']
    new_version = conf_obj.version
    cur_version = get_cluster_version(cid, pillar)

    ensure_version_allowed(MY_CLUSTER_TYPE, new_version)
    validate_version_change(
        cid=cluster_obj['cid'], pillar=pillar, new_version=conf_obj.version, current_version=cur_version
    )

    metadb.set_default_versions(
        cid=cid,
        subcid=None,
        shard_id=None,
        env=env,
        major_version=new_version,
        edition=cur_version.edition,
        ctype=MY_CLUSTER_TYPE,
    )
    metadb.update_cluster_pillar(cluster_obj['cid'], pillar)

    return OperationParameters(changes=True, task_args={}, time_limit=timedelta(hours=1))


def handle_fcv_change(
    conf_obj: MongodbConfigSpec, pillar: MongoDBPillar, cluster_obj: dict, cur_version: Version
) -> OperationParameters:
    """
    Facilitate version change
    """

    assert conf_obj.feature_compatibility_version is not None

    validate_fcv(conf_obj.feature_compatibility_version, cur_version)
    pillar.feature_compatibility_version = conf_obj.feature_compatibility_version
    metadb.update_cluster_pillar(cluster_obj['cid'], pillar)

    return OperationParameters(changes=True, task_args={}, time_limit=timedelta(hours=1))


def _modify_mongodb_cluster(
    cluster_obj: dict, _schema: Schema, config_spec: dict
) -> Tuple[MongoDBTasks, bool, dict, timedelta]:
    """
    Modifies MongoDB cluster. changes flag and task_args

    Logic is separated into three steps:
    1. Determine if there are version changes.
    2. Determine if there are fcv changes.
    3. Determine if there are service spec changes.
    """

    task_type = MongoDBTasks.modify

    try:
        conf_obj = MongodbConfigSpec(config_spec, version_required=False)
    except (ValueError, KeyError) as err:
        raise ParseConfigError(err)

    pillar = get_cluster_pillar(cluster_obj)
    cid = cluster_obj['cid']
    op_params = OperationParameters(changes=False, task_args={}, time_limit=timedelta(hours=1))

    # Version is tricky: its always there, so we need to compare it with current situation.
    cur_version = get_cluster_version(cid, pillar)
    if conf_obj.version is not None and conf_obj.version != cur_version:
        version_params = handle_version_change(conf_obj, pillar, cluster_obj)
        if version_params.changes:
            task_type = MongoDBTasks.upgrade
            op_params += version_params

    fcv = conf_obj.feature_compatibility_version
    if fcv is not None and fcv != pillar.feature_compatibility_version:
        op_params += handle_fcv_change(conf_obj, pillar, cluster_obj, cur_version)

    update_pillar = False
    for field in ('access', 'performance_diagnostics'):
        field_updated = update_field(field, conf_obj, pillar)
        update_pillar |= field_updated
        if field_updated:
            op_params += OperationParameters(
                changes=True, task_args={'include-{}'.format(field): True}, time_limit=timedelta(hours=0)
            )

    update_backup_schedule = False
    if conf_obj.backup_start or conf_obj.backup_retain_period:

        backup_start = cluster_obj['backup_schedule'].get('start', {})
        backup_retain_period = cluster_obj['backup_schedule'].get('retain_period')

        new_backup_start = conf_obj.backup_start
        new_backup_retain_period = conf_obj.backup_retain_period

        if new_backup_start and new_backup_start != backup_start:
            backup_start = new_backup_start
            update_backup_schedule = True

        if new_backup_retain_period and new_backup_retain_period != backup_retain_period:
            backup_retain_period = new_backup_retain_period
            update_backup_schedule = True

        if update_backup_schedule:
            metadb.add_backup_schedule(
                cluster_obj['cid'],
                get_backup_schedule(
                    MY_CLUSTER_TYPE,
                    backup_start,
                    backup_retain_period,
                    cluster_obj['backup_schedule'].get('use_backup_service', None),
                ),
            )

    if update_pillar:
        metadb.update_cluster_pillar(cluster_obj['cid'], pillar)

    if update_pillar or update_backup_schedule:
        op_params += OperationParameters(
            changes=True, task_args={'include-metadata': True}, time_limit=timedelta(hours=1)
        )

    if conf_obj.has_services():
        op_params += handle_service_spec_changes(conf_obj, _schema, pillar, cluster_obj)

    return task_type, op_params.changes, op_params.task_args, op_params.time_limit


@register_request_handler(MY_CLUSTER_TYPE, Resource.CLUSTER, DbaasOperation.MODIFY)
def modify_mongodb_cluster(
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
    Modifies MongoDB cluster. Return Operation
    """
    changes = False
    task_type = MongoDBTasks.modify
    task_args = {}  # type: dict
    time_limit = None

    if config_spec is not None:
        task_type, changes, task_args, time_limit = _modify_mongodb_cluster(
            cluster_obj=cluster_obj, _schema=_schema, config_spec=config_spec
        )

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
            operation_type=MongoDBOperations.modify,
            metadata=ModifyClusterMetadata(),
            cid=cluster_obj['cid'],
            time_limit=time_limit,
            task_args=task_args,
        )

    raise NoChangesError()
