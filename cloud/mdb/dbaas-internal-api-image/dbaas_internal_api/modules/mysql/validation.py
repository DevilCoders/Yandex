# -*- coding: utf-8 -*-
"""
DBaaS Internal API MySQL cluster validation
"""

from ...core.exceptions import DbaasClientError, DbaasNotImplementedError
from ...utils.feature_flags import ensure_feature_flag
from ...utils.version import Version
from .traits import MySQLOperations, MySQLTasks
from .constants import SQL_MODES_8_0
from .pillar import MySQLPillar

UPGRADE_TASKS = {
    '8.0': MySQLTasks.upgrade80,
}

UPGRADE_TASK_TO_OPERATION = {
    MySQLTasks.upgrade80: MySQLOperations.upgrade80,
}


def change_sql_mode(pillar: MySQLPillar):
    cfg = pillar.config.get_config()
    sql_mode = cfg.get('sql_mode')
    result = []
    changed = False
    if sql_mode is not None:
        for mode in sql_mode:
            # remove unsupported SQL_MODEs
            if mode in SQL_MODES_8_0:
                result.append(mode)
            else:
                changed = True

    if changed:
        pillar.config.merge_config({'sql_mode': result})
    return changed


VERSION_UPGRADE_5_7_TO_8_0_FEATURE_FLAG = 'MDB_MYSQL_5_7_TO_8_0_UPGRADE'

UPGRADE_PATHS: dict[str, dict] = {
    '8.0': {'from': ['5.7'], 'feature_flag': VERSION_UPGRADE_5_7_TO_8_0_FEATURE_FLAG, 'migrations': [change_sql_mode]},
}


def validate_version_upgrade(version_raw: str, new_version_raw: str):
    """
    Check if upgrade from version_raw to new_version_raw is possible for cluster
    """
    upgrade_not_supported_msg = f'Upgrade from {version_raw} to {new_version_raw} is not supported'
    version, new_version = Version.load(version_raw), Version.load(new_version_raw)
    # todo: [MDB-13009] replace with updatable_to logic
    version_path = UPGRADE_PATHS.get(new_version.to_string())

    if version > new_version:
        raise DbaasClientError('Version downgrade detected')
    try:
        if version_raw not in version_path['from']:  # type: ignore
            raise DbaasNotImplementedError(upgrade_not_supported_msg)
        feature_flag = version_path.get('feature_flag')  # type: ignore
        if feature_flag is not None:
            ensure_feature_flag(feature_flag)  # type: ignore
    except KeyError:
        raise DbaasNotImplementedError(upgrade_not_supported_msg)


def validate_mysql_repl_source(pillar, hosts_repl_src):

    config = pillar.config.get_config()
    wait_slaves = config.get('rpl_semi_sync_master_wait_for_slave_count', 1)
    # mysync can manage cases with rpl_semi_sync_master_wait_for_slave_count = 1
    if wait_slaves == 1:
        return True

    ha_cnt_after = 0
    for _, repl_source in hosts_repl_src.items():
        if not repl_source:
            ha_cnt_after += 1

    if wait_slaves > ha_cnt_after - 1:
        raise DbaasClientError(
            'not enough HA replicas remains! Should be not less than rpl_semi_sync_master_wait_for_slave_count (now {wait_slaves})'.format(
                wait_slaves=wait_slaves
            )
        )

    return True


def fix_pillar_on_upgrade(old_version: Version, new_version: Version, pillar: MySQLPillar):
    version_path = UPGRADE_PATHS.get(new_version.to_string(), {})
    migrations = version_path.get('migrations', [])
    changed = False
    for migration in migrations:
        changed |= migration(pillar)
    return changed
