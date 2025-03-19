import json
import typing
from dataclasses import dataclass
from datetime import datetime
from enum import EnumMeta

from cloud.ai.lib.python.serialization import YsonSerializable, OrderedYsonSerializable, unwrap_optional


@dataclass
class TableMeta(YsonSerializable):
    dir_path: str
    attrs: dict


type_mapping = {
    int: 'int64',
    float: 'double',
    str: 'string',
    bytes: 'string',
    bool: 'boolean',
    datetime: 'string',
}


def objects_to_rows(objects: typing.List[OrderedYsonSerializable], sort: bool = True) -> typing.List[dict]:
    if sort:
        objects = sorted(objects)
    return [r.to_yson() for r in objects]


def single_object_to_row(object: typing.Optional[YsonSerializable]) -> dict:
    return {} if object is None else object.to_yson()


def get_table_name(date: datetime) -> str:
    return f'{date.year}-{date.month:02d}-{date.day:02d}'


def generate_attrs(
    cls: typing.ClassVar[OrderedYsonSerializable],
    unique_keys=True,
    required: typing.Optional[typing.Set[str]] = None,
    type_hints: typing.Optional[typing.Dict[str, str]] = None,
) -> dict:
    schema = []

    attrs_map = dict(cls.__annotations__)  # we rely on annotation field order here
    for attr_name, attr_value in cls.serialization_additional_fields().items():
        attrs_map[attr_name] = type(attr_value)

    for attr_name, attr_type in attrs_map.items():
        attr_type = unwrap_optional(attr_type)
        if type_hints is not None and attr_name in type_hints:
            t = type_hints[attr_name]
        elif type(attr_type) == EnumMeta:
            t = 'string'
        else:
            t = type_mapping.get(attr_type, 'any')
        entry = {'name': attr_name, 'type': t}
        if required is not None and attr_name in required:
            entry['required'] = True
        schema.append(entry)

    name_to_type = {x['name']: x['type'] for x in schema}

    for schema_entry in schema:
        name = schema_entry['name']
        if name not in cls.primary_key():
            continue
        if name_to_type[name] == 'any':
            raise ValueError(f'"{name}" can\'t be a part of primary key')
        schema_entry['sort_order'] = 'ascending'
        schema_entry['required'] = True

    # key columns should be first in schema, we move them up in
    # inefficient bubble-sort manner to make outcome more predictable
    for i, name in enumerate(cls.primary_key()):
        j = -1
        for k, schema_entry in enumerate(schema):
            if schema_entry['name'] == name:
                j = k
                break
        for k in range(j, i, -1):
            schema[k], schema[k - 1] = schema[k - 1], schema[k]

    return {
        'schema': {
            '$value': schema,
            '$attributes': {'strict': True, 'unique_keys': unique_keys},
        }
    }


# For Nirvana operation "Append rows to YT table"
def generate_json_options_for_table(meta: TableMeta, name: str) -> dict:
    return {
        'table-meta': json.dumps(meta.to_yson(), indent=4, ensure_ascii=False),
        'table-name': name,
    }
