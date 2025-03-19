"""
Wrappers around yatest package
"""

import os
from pathlib import Path
from typing import Dict


def locate_pg_dump() -> str:
    """
    Return pg_dump binary path
    """
    try:
        # pylint: disable=import-error
        import yatest.common

        return yatest.common.work_path('pg/bin/pg_dump')
    except ModuleNotFoundError:
        return 'pg_dump'


def define_pg_env() -> Dict[str, str]:
    """
    Fill pg specific environment vars.

    In A we should add Postgre lib to LD_LIBRARY_PATH

    .../bin/pg_dump: error while loading shared libraries: libpq.so.5:
        cannot open shared object file: No such file or directory
    """
    env = os.environ.copy()
    try:
        # pylint: disable=import-error
        import yatest.common

        lib_dir = yatest.common.work_path('pg/lib')
        if 'LD_LIBRARY_PATH' in env:
            env['LD_LIBRARY_PATH'] += ':' + lib_dir
        else:
            env['LD_LIBRARY_PATH'] = lib_dir
        # tiny workaround for OSX 'system' Postgre
        #  > pg_dump: server version: 11.6; pg_dump version: 11.6
        if 'DYLD_FALLBACK_LIBRARY_PATH' in env:
            env['DYLD_FALLBACK_LIBRARY_PATH'] += ':' + lib_dir
        else:
            env['DYLD_FALLBACK_LIBRARY_PATH'] = lib_dir
    except ModuleNotFoundError:
        pass
    return env


def locate_pgmirate() -> str:
    """
    Return pgmigrate command.

    Binary - when test running in A.
    Just `pgmigrate` without any check elsewhere
    """
    try:
        # pylint: disable=import-error
        import yatest.common

        return yatest.common.binary_path('contrib/python/yandex-pgmigrate/bin/pgmigrate')
    except ModuleNotFoundError:
        return 'pgmigrate'


def locate_populate_table() -> str:
    """
    Return populate command
    """
    try:
        # pylint: disable=import-error
        import yatest.common

        return yatest.common.binary_path('cloud/mdb/dbaas_metadb/bin/populate_table')
    except ModuleNotFoundError:
        pp_path = Path(__file__).resolve().parent.parent.parent / 'bin' / 'populate_table.py'
        return str(pp_path)
