# coding: utf-8
"""
Project utils
"""

import os
from pathlib import Path
from typing import Dict


class MissingRecipeVarsError(Exception):
    """
    missing recipe var
    """


def get_recipe_dsn_from_env() -> Dict[str, str]:
    """
    Get metadb env vars
    """
    dsn = {}
    for env_var, dsn_var in {
        'METADB_POSTGRESQL_RECIPE_HOST': 'host',
        'METADB_POSTGRESQL_RECIPE_PORT': 'port',
    }.items():
        try:
            dsn[dsn_var] = os.environ[env_var]
        except KeyError:
            raise MissingRecipeVarsError(f"Variable '{env_var}' not found")
    return dsn


def is_running_with_recipe() -> bool:
    """
    return true if we running with metadb recipe
    """
    try:
        get_recipe_dsn_from_env()
        return True
    except MissingRecipeVarsError:
        pass
    return False


is_running_in_a = is_running_with_recipe


def project_root() -> Path:
    """
    return path to dbaas-internal-api project
    """
    try:
        # pylint: disable=import-error, import-outside-toplevel
        import yatest.common

        return Path(yatest.common.source_path('cloud/mdb/mdb-internal-api'))
    except ImportError:
        return Path(__file__).parent.parent.parent


def func_tests_root() -> Path:
    """
    return path to functional tests root
    """
    return project_root() / 'functest'


def tmp_root() -> Path:
    """
    return path to tmp dir
    """
    root = os.getenv('DBAAS_INTERNAL_API_RECIPE_TMP_ROOT')
    if root:
        return Path(root)

    return func_tests_root() / 'tmp'


def logs_root() -> Path:
    """
    return path to logs
    """

    root = os.getenv('DBAAS_INTERNAL_API_RECIPE_LOGS_ROOT')
    if root:
        return Path(root)

    return project_root() / 'logs'


def generated_data_root() -> Path:
    """
    return path to generated data
    """
    try:
        # pylint: disable=import-error, import-outside-toplevel
        import yatest.common

        return Path(yatest.common.output_path('generated_data_root'))
    except ImportError:
        return func_tests_root() / 'data' / 'generated'
