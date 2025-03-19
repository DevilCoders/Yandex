"""
Module describes helpers for various info display-related functions
"""

from typing import Optional, Type

from .helpers import copy_merge_dict
from .types import ConfigSet
from .validation import Schema
from .version import Version


def render_config_set(
    default_config: dict,
    user_config: dict,
    schema: Type[Schema],
    instance_type_obj: dict,
    version: Optional[Version] = None,
    **kwargs,
) -> ConfigSet:
    """
    Assemble ConfigSet object.
    """
    schema_obj = schema(instance_type=instance_type_obj, version=version, **kwargs)
    default_data = schema_obj.dump(default_config).data
    variable_default_config = schema_obj.load(default_data).data
    effective_config = copy_merge_dict(variable_default_config, user_config)
    return ConfigSet(effective=effective_config, user=user_config, default=variable_default_config)
