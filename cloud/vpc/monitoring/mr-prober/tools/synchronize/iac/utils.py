from enum import Enum

import yaml
from yaml.constructor import ConstructorError

__all__ = ["UniqueKeySafeLoader"]


class UniqueKeySafeLoader(yaml.SafeLoader):
    """
    Special (safe) loader with duplicate key checking. Example os usage:

    yaml.load(data, Loader=utils.UniqueKeySafeLoader)

    Raises ConstructionError if duplicates found.
    """

    def construct_mapping(self, node, deep=False):
        mapping = []
        for key_node, value_node in node.value:
            key = self.construct_object(key_node, deep=deep)
            if key in mapping:
                raise ConstructorError(
                    "while constructing a mapping", node.start_mark,
                    "found duplicate key (%s) in YAML" % key, key_node.start_mark
                )
            mapping.append(key)
        return super().construct_mapping(node, deep)


class EnumSupportedSafeDumper(yaml.SafeDumper):
    def __init__(self, *args, **kwargs):
        super().__init__(*args, **kwargs)
        self.add_multi_representer(Enum, self.represent_enum)

    @staticmethod
    def represent_enum(dumper, data) -> str:
        return dumper.represent_data(data.value)
