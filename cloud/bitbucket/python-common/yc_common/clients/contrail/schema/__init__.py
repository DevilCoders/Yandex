"""This module creates schema data structure by parsing Contrail
schema files.

>>> list_available_schema_version()
>>> schema = create_schema_from_version("2.21")
>>> schema.all_resources()
>>> schema.resource('virtual-network').children

"""
import importlib
import pkgutil

from typing import List, Optional

from yc_common import logging
from yc_common.clients.contrail.schema.exceptions import (
    SchemaError, SchemaVersionNotAvailable, ResourceNotDefined
)
from yc_common.clients.contrail.schema.schema import (
    Schema, ResourceProperty, ResourceSchema, DummySchema, DummyResourceSchema
)

log = logging.get_logger(__name__)
log.setLevel(logging.INFO)  # Disable excessive logging

MODULENAME_PREFIX = '_v_'


def get_last_schema_version() -> Optional[str]:
    """Return the last available schema version. Version are
    lexicographically sorted. If no version is available, return None.
    """
    versions = list_available_schema_version()
    if len(versions) > 0:
        return sorted(versions)[-1]
    else:
        return None


def list_available_schema_version() -> List[str]:
    """To discover available schema versions."""
    return [
        _modulename_to_version(name)
        for _, name, _ in pkgutil.iter_modules(__path__)
        if _is_version_modulename(name)
    ]


def create_schema_from_version(version: str) -> Schema:
    """Provide a version of the schema to create it. Use
    list_available_schema_version to discover available versions.

    This code was generated with "pydump-schema" plugin for contrail-api-cli:
    https://bb.yandex-team.ru/projects/CLOUD/repos/contrail-yandex/browse/contrail-api-cli-pydump-schema
    """
    module_name = _version_to_modulename(version)

    try:
        module = importlib.import_module('{}.{}'.format(__name__, module_name))
    except ImportError:
        raise SchemaVersionNotAvailable(version)
    return module.schema


def _is_version_modulename(modulename: str) -> bool:
    return modulename.startswith(MODULENAME_PREFIX)


def _version_to_modulename(version: str) -> str:
    return "{}{}".format(MODULENAME_PREFIX, version.replace('.', '_'))


def _modulename_to_version(modulename: str) -> str:
    return modulename.replace(MODULENAME_PREFIX, '', 1).replace('_', '.')
