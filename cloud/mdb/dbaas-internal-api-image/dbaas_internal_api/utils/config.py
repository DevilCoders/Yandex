# -*- coding: utf-8 -*-
"""
DBaaS Internal API utils for config-related operations
Contains some config-getters
"""
from copy import deepcopy
from typing import Iterable, List

from flask import current_app

from .feature_flags import has_feature_flag


def default_cloud_quota():
    """
    Returns default cloud quota for creation
    """
    return current_app.config['DEFAULT_CLOUD_QUOTA']


def cluster_type_config(cluster_type):
    """
    Return cluster type specific settings
    """
    return current_app.config['CTYPE_CONFIG'][cluster_type]


def get_domain(vtype):
    """
    Returns domain from config for given vm type
    """
    return current_app.config['VTYPES'][vtype]


def get_prefix(geo):
    """
    Returns domain name prefix for given geo
    """
    return current_app.config['ZONE_RENAME_MAP'].get(geo, geo)


def available_environments(cluster_type: str) -> Iterable[str]:
    """
    Returns list of available environments for given cluster type
    """
    return current_app.config['ENVCONFIG'].get(cluster_type, {}).keys()


def get_environment_config(cluster_type: str, env: str) -> dict:
    """
    Get environment config
    """
    return current_app.config['ENVCONFIG'][cluster_type][env]


def is_auth_user_enabled(cluster_type):
    """
    Returns CREATE_AUTH_USER value from config for cluster_type
    """
    return current_app.config['CREATE_AUTH_USER'].get(cluster_type, False)


def environment_mapping():
    """
    Maps internal env names to external representation, like dev ->
    development
    """
    return current_app.config['ENV_MAPPING']


def get_console_address():
    """
    Returns http address of WebUI console
    """
    return current_app.config['CONSOLE_ADDRESS']


def cluster_charts(cluster_type):
    """
    Returns list of tuples(name, description, url template) for cluster charts
    """
    return current_app.config['CHARTS'].get(cluster_type)


def s3_config():
    """
    Returns s3 config
    """
    return current_app.config['S3']


def boto_config():
    """
    Returns boto config
    """
    return current_app.config['BOTO_CONFIG']


def get_bucket_name(cid: str) -> str:
    """
    Returns bucket name for cluster
    """
    return '{prefix}{cid}'.format(prefix=current_app.config['BUCKET_PREFIX'], cid=cid)


def get_background_logger_name() -> str:
    """
    Returns background logger name
    """
    return current_app.config['LOGCONFIG_BACKGROUND_LOGGER']


def get_close_path() -> str:
    """
    Returns path of close-file from config
    """
    return current_app.config['CLOSE_PATH']


def get_disk_type_id(vtype: str) -> str:
    """
    Returns disk_type_id for vtype
    """
    return current_app.config['DEFAULT_DISK_TYPE_IDS'][vtype]


def get_password_length() -> int:
    """
    Returns length of generated passwords.
    """
    return current_app.config['PASSWORD_LENGTH']


def get_minimal_disk_unit() -> int:
    """
    Returns minimal disk size unit
    """
    return int(current_app.config['MINIMAL_DISK_UNIT'])


def get_task_expose() -> bool:
    """
    Return task errors expose setting
    """
    return current_app.config['EXPOSE_ALL_TASK_ERRORS']


def vtypes_with_networks():
    """
    Get list of vtypes with networks management enabled
    """
    return current_app.config['NETWORK_VTYPES']


def get_backups_purge_delay() -> int:
    """
    Returns how many days backup is accessible after
    cluster delete
    """
    return current_app.config.get('BACKUPS_PURGE_DELAY', 7)


def get_decommissioning_zones(force: bool = False) -> List[str]:
    """
    Return geos in which we should not create and upscale new hosts
    """
    if has_feature_flag('MDB_ALLOW_DECOMMISSIONED_ZONE_USE') and not force:
        return []
    return current_app.config['DECOMMISSIONING_ZONES']


def get_decommissioning_flavors(force: bool = False) -> List[str]:
    """
    Return flavors, which shouldn't be used for
    cluster creation or modification resource preset to
    """
    if has_feature_flag('MDB_ALLOW_DECOMMISSIONED_FLAVOR_USE') and not force:
        return []
    return current_app.config['DECOMMISSIONING_FLAVORS']


def get_console_default_resources(cluster_type) -> dict:
    """
    Returns default resources for console
    """
    generation_names = get_generation_names()
    ret = current_app.config['CONSOLE_DEFAULT_RESOURCES'][cluster_type].copy()
    for i in ret.values():
        i['generation_name'] = generation_names[i['generation']]
    return ret


def get_decommission_timeout_boundaries() -> dict:
    """
    Returns min and max value for decommission timeout
    """
    if 'DECOMMISSION_TIMEOUT' in current_app.config:
        return current_app.config['DECOMMISSION_TIMEOUT'].copy()
    return {'min': 0, 'max': 86400}


def get_default_resources() -> dict:
    """
    Returns default subcluster resources
    """
    return deepcopy(current_app.config['HADOOP_DEFAULT_RESOURCES'])


def get_generation_names() -> dict:
    """
    Returns map of generation id to human name
    """
    # Application expects that keys within GENERATION_NAMES are integers. Within k8s we pass configuration as env
    # variables whose values are json-encoded. But there's no way to build json with integers as keys. That's why
    # we need to convert string keys to integers here.
    if 'GENERATION_NAMES_FIXED' not in current_app.config:
        current_app.config['GENERATION_NAMES_FIXED'] = {
            int(k): v for k, v in current_app.config['GENERATION_NAMES'].items()
        }
    return current_app.config['GENERATION_NAMES_FIXED']


def is_dispenser_used() -> bool:
    """
    Returns dispenser use flag
    """
    return current_app.config['DISPENSER_USED']


def get_internal_schema_fields_expose() -> bool:
    """
    Returns internal schema fields expose flag
    """
    return current_app.config['INTERNAL_SCHEMA_FIELDS_EXPOSE']


def get_dataproc_manager_public_config() -> dict:
    """
    Returns dataproc manager public config
    """
    return deepcopy(current_app.config['DATAPROC_MANAGER_PUBLIC_CONFIG'])


def minimal_zk_resources() -> List[dict]:
    """
    Returns required number of cores for each ZK node for different CH subcluster configurations.
    """
    return current_app.config['MINIMAL_ZK_RESOURCES']


def external_uri_validation() -> dict:
    """
    Return EXTERNAL_URI_VALIDATION config section.
    """
    return current_app.config['EXTERNAL_URI_VALIDATION']


def version_validator() -> dict:
    """
    Return VERSION_VALIDATOR config section.
    """
    return current_app.config['VERSION_VALIDATOR']


def dataproc_joblog_config() -> dict:
    """
    Return DATAPROC_JOBLOG_CONFIG config section.
    """
    return current_app.config['DATAPROC_JOBLOG_CONFIG']


def get_default_backup_schedule(cluster_type: str) -> dict:
    """
    Return default backup schedule for given cluster type.
    """
    return deepcopy(current_app.config['DEFAULT_BACKUP_SCHEDULE'][cluster_type])


def get_e2e_config() -> dict:
    """
    Returns e2e config with folder_ext_id and cluster_name.
    """
    return current_app.config.get('E2E', {})


def get_postgresql_max_ha_hosts() -> int:
    """
    Returns limit of postgresql HA hosts in cluster.
    """
    return current_app.config.get('POSTGRESQL_MAX_HA_HOSTS', 17)
