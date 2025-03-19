"""Database migration utils"""

import time
import random
import importlib
import os
import re

from typing import Callable, Optional
from schematics.types import IntType, BooleanType, StringType
from schematics import common as schematics_common

from yc_common import logging, misc
from yc_common.models import Model
from yc_common.clients.kikimr.client import KikimrTableSpec, KikimrTable, KikimrDataType, get_kikimr_client
from yc_common.clients.kikimr.exceptions import TableNotFoundError, KikimrError
from yc_common.clients.kikimr.util import retry_idempotent_kikimr_errors
from yc_common.exceptions import LogicalError, Error

from . import utils


class DbSchemeVersionName:
    CURRENT = "current"
    STAGING = "staging"

    ALL = (CURRENT, STAGING)


# this version is public, and should be used when db setup is experimental and not supposed to be migrating
DB_SCHEME_VERSION_EXPERIMENTAL = -1
_ID = "id"

# sleep time, to retry for new scheme db version
_RETRY_FOR_SCHEME_SLEEP_TIME = 10
_RETRY_FOR_SCHEME_SLEEP_TIME_JITTER = 5

_PREPARE_TOOL_REGISTRY = {}  # Set of tools that may be executed during process start
_POPULATE_TOOL_REGISTRY = {}  # Set of tools that may be executed in background after process start

log = logging.get_logger(__name__)


# Db scheme versioning support


class _DbSchemeInfo(Model):
    id = StringType(required=True)
    # old columns FIXME: Remove after transition period
    version = IntType(export_level=schematics_common.DROP)
    populated = BooleanType(export_level=schematics_common.DROP)
    # new columns
    prepared_version = IntType(required=True, strict=True)  # min_value=DB_SCHEME_VERSION_EXPERIMENTAL
    populated_version = IntType(required=True, strict=True)  # min_value=DB_SCHEME_VERSION_EXPERIMENTAL


def _db_scheme_table(database_id: str) -> KikimrTable:
    return KikimrTable(
        database_id=database_id,
        table_name="_scheme_version",
        table_spec=KikimrTableSpec(
            columns={
                "id": KikimrDataType.UTF8,
                # old columns FIXME: Remove after transition period
                "version": KikimrDataType.INT64,
                "populated": KikimrDataType.BOOL,
                # new columns
                "prepared_version": KikimrDataType.INT64,
                "populated_version": KikimrDataType.INT64,
            },
            primary_keys=["id"]
        ),
        model=_DbSchemeInfo)


@retry_idempotent_kikimr_errors
def _update_db_scheme_info(database_id: str, *, prepared_version: int=None, populated_version: int=None):
    table = _db_scheme_table(database_id=database_id)
    update = misc.drop_none({
        "prepared_version": prepared_version,
        "populated_version": populated_version,
    })
    if not update:
        return
    with table.client.transaction() as tx:
        tx.update_object("UPDATE $table $set WHERE id = ?", update, _ID)


def _get_db_scheme_info(database_id: str, only_if_exists=False, ignore_kikimr_errors=False, tx=None) -> Optional[_DbSchemeInfo]:
    if tx is None:
        tx = get_kikimr_client(database_id)

    with tx.table_scope(_db_scheme_table(database_id)):
        try:
            info = tx.select_one("SELECT * from $table", model=_DbSchemeInfo)
        except TableNotFoundError:
            if only_if_exists:
                return None
            else:
                raise
        except KikimrError as e:
            if ignore_kikimr_errors:
                log.error("Database query has failed: %s", e)
                return None
            else:
                raise

        if info is None and not only_if_exists:
            raise Error("No version schema found in {!r} database", database_id)

        return info


def _get_db_scheme_populated_version(database_id: str, only_if_exists=False, ignore_kikimr_errors=False, tx=None):
    info = _get_db_scheme_info(database_id, only_if_exists=only_if_exists, ignore_kikimr_errors=ignore_kikimr_errors, tx=tx)
    return info.populated_version if info is not None else None


def _get_db_scheme_prepared_version(database_id: str, only_if_exists=False, ignore_kikimr_errors=False, tx=None):
    info = _get_db_scheme_info(database_id, only_if_exists=only_if_exists, ignore_kikimr_errors=ignore_kikimr_errors, tx=tx)
    return info.prepared_version if info is not None else None


def wait_for_db_scheme_version(database_id: str, version: int):
    log.info("Start waiting for %r database scheme version %r...", database_id, version)
    while True:
        populated_version = _get_db_scheme_info(database_id, only_if_exists=True, ignore_kikimr_errors=True)

        # we should wait more, if query failed, or current db doesn't have scheme version.
        # we can get valid response next time, or someone can populate db for us
        if populated_version is None:
            log.info("waiting for valid database scheme in %r", database_id)
            continue

        if version == populated_version.prepared_version and version >= populated_version.populated_version:
            break
        elif version < populated_version.prepared_version and version == populated_version.populated_version:
            break
        else:
            log.info("Service version %r, %r database version (prepared %r, populated %r). Waiting...",
                     version, database_id, populated_version.prepared_version, populated_version.populated_version)

        _sleep()

    log.info("End waiting for %r database scheme versions %r (prepared=%d, populated=%d).",
             database_id, version, populated_version.prepared_version, populated_version.populated_version)


def erase_db_scheme_version(database_id: str):
    table = _db_scheme_table(database_id=database_id)
    table.drop(only_if_exists=True)


@retry_idempotent_kikimr_errors
def populate_db_scheme_version(database_id: str, version: int):
    current_info = _get_db_scheme_info(database_id, only_if_exists=True)
    if current_info is not None:
        log.info("Db scheme already populated! Database id: %r, scheme version: %r", database_id, current_info.version)
        return

    table = _db_scheme_table(database_id=database_id)
    table.create()
    table.client.insert_object("REPLACE INTO $table", _DbSchemeInfo.new(
        id=_ID,
        version=version,
        populated=True,
        prepared_version=version,
        populated_version=version,
    ))


# Migration process variants


def _inc_prepared_version(database_id, current_version):
    next_version = current_version + 1
    log.info("Running prepare for %r from %r version to %r version.",
             database_id, current_version, next_version)
    run_prepare_tool(database_id, next_version)
    _update_db_scheme_info(database_id, prepared_version=next_version)
    return next_version


def _inc_populated_version(database_id, current_version):
    next_version = current_version + 1
    log.info("Running populate for %r from %r version to %r version.",
             database_id, current_version, next_version)
    run_populate_tool(database_id, next_version)
    _update_db_scheme_info(database_id, populated_version=next_version)
    return next_version


def offline_migrate(database_id, target_version):
    """
    Offline migration with full service stop

    Can be used only for migrate database schema from previous setup
    """

    current_prepared_version = _get_db_scheme_prepared_version(database_id)
    current_populated_version = _get_db_scheme_populated_version(database_id)

    current_version = min(current_prepared_version, current_populated_version)

    # do not allow migration from/to staging db scheme
    if DB_SCHEME_VERSION_EXPERIMENTAL in (current_version, target_version) or target_version < current_version:
        raise Error("Unable to migrate {!r} from version {!r} to version {!r}", database_id, current_version, target_version)

    while current_version < target_version:
        current_version += 1
        if current_prepared_version < current_version:
            current_prepared_version = _inc_prepared_version(database_id, current_prepared_version)

        if current_populated_version < current_version:
            current_populated_version = _inc_populated_version(database_id, current_populated_version)


def online_prepare(database_id, target_version):
    """Prepare online migration
    """

    # FIXME: Remove after transition period
    utils.alter_table_add_columns(_db_scheme_table(database_id))

    current_version = _get_db_scheme_prepared_version(database_id)

    # do not allow migration from/to staging db scheme
    if DB_SCHEME_VERSION_EXPERIMENTAL in (current_version, target_version) or target_version < current_version:
        raise Error("Unable to migrate {!r} from version {!r} to version {!r}", database_id, current_version, target_version)

    while current_version < target_version:
        current_version = _inc_prepared_version(database_id, current_version)


def online_populate(database_id, target_version):
    prepared_version = _get_db_scheme_prepared_version(database_id)
    if prepared_version != target_version:
        raise Error("Database is not prepared yet: current version {!r}, required version {!r}",
                    prepared_version, target_version)

    current_version = _get_db_scheme_populated_version(database_id)

    # do not allow migration from/to staging db scheme
    if DB_SCHEME_VERSION_EXPERIMENTAL in (current_version, target_version) or target_version < current_version:
        raise Error("Unable to migrate {!r} from version {!r} to version {!r}", database_id, current_version, target_version)

    while current_version < target_version:
        current_version = _inc_populated_version(database_id, current_version)


# Registry of migration tools


def load_all_migration_tools(from_dir: str, package: str):
    for name in os.listdir(from_dir):
        match = re.search(r"^(v\d+)\.py[co]?$", name)
        if match:
            importlib.import_module("." + match.group(1), package=package)


def run_prepare_tool(database_id: str, to_version: int) -> Optional[Callable[[], None]]:
    tool = _PREPARE_TOOL_REGISTRY.get(database_id, {}).get(to_version)
    if tool is None:
        raise LogicalError("No prepare tool for database {!r} of version {!r} was found.", database_id, to_version)
    else:
        tool()


def run_populate_tool(database_id: str, to_version: int) -> Optional[Callable[[], None]]:
    tool = _POPULATE_TOOL_REGISTRY.get(database_id, {}).get(to_version)
    if tool is None:
        raise LogicalError("No populate tool for database {!r} of version {!r} was found.", database_id, to_version)
    else:
        tool()


def register_prepare_tool(database_id: str, version: int):
    db_registry = _PREPARE_TOOL_REGISTRY.setdefault(database_id, {})

    def wrapper(func: Callable[[], None]):
        if db_registry.setdefault(version, func) is not func:
            raise LogicalError(
                "DB prepare tool for {!r} database with {!r} version already registered.", database_id, version)
        return func

    return wrapper


def register_populate_tool(database_id: str, version: int):
    db_registry = _POPULATE_TOOL_REGISTRY.setdefault(database_id, {})

    def wrapper(func: Callable[[], None]):
        if db_registry.setdefault(version, func) is not func:
            raise LogicalError(
                "DB populate tool for {!r} database with {!r} version already registered.", database_id, version)
        return func

    return wrapper


register_migration_tool = register_prepare_tool  # For backward compatibility


# Misc helpers

def _sleep():
    time.sleep(_RETRY_FOR_SCHEME_SLEEP_TIME + random.randint(-_RETRY_FOR_SCHEME_SLEEP_TIME_JITTER,
                                                             _RETRY_FOR_SCHEME_SLEEP_TIME_JITTER))
