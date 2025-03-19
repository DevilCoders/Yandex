import abc
import json
import typing
from datetime import datetime, timedelta
from enum import Enum, EnumMeta

from cloud.ai.lib.python.datetime import format_datetime, parse_datetime, format_timedelta, parse_timedelta

"""
Serializes object to dict, only for annotated members of type:
- primitive types: str, bytes, int, float, bool
- dicts, but they are expected to contain only primitive types to be YSON serializable
- enums
- nested yson serializable classes
- typing.Optional
- datetime
- typing.List with type equal one of above

Originally used to store object as YT table rows.

It is almost JSON serialization except bytes members support.
"""


class YsonSerializable(abc.ABC):
    def to_yson(self) -> dict:
        y = {}
        for field_name, field_value in self.__dict__.items():
            if field_name not in _get_all_annotations(type(self)):
                continue
            if isinstance(field_value, list):
                y[field_name] = [member_to_yson(value) for value in field_value]
            else:
                y[field_name] = member_to_yson(field_value)
        y.update(self.serialization_additional_fields())
        return y

    @classmethod
    def from_yson(cls, fields: dict) -> 'YsonSerializable':
        return yson_to_obj(fields, cls)

    @staticmethod
    def serialization_additional_fields() -> typing.Dict[str, typing.Any]:
        return {}

    @staticmethod
    def deserialization_overrides() -> typing.Dict[str, typing.Callable]:
        return {}


class OrderedYsonSerializable(YsonSerializable):
    @staticmethod
    @abc.abstractmethod
    def primary_key() -> typing.List[str]:
        pass

    def __lt__(self, other):
        for field_name in self.primary_key():
            if getattr(self, field_name) == getattr(other, field_name):
                continue
            else:
                return getattr(self, field_name) < getattr(other, field_name)
        return False


def member_to_yson(member_value):
    if member_value is None:
        return None
    elif isinstance(member_value, YsonSerializable):
        return member_value.to_yson()
    elif isinstance(member_value, Enum):
        return member_value.value
    elif isinstance(member_value, datetime):
        return format_datetime(member_value)
    elif isinstance(member_value, timedelta):
        return format_timedelta(member_value)
    elif _is_primitive(member_value):
        return member_value
    else:
        raise ValueError(f'Unsupported member type: {type(member_value)}')


def yson_to_obj(fields, cls):
    overrides = cls.deserialization_overrides()
    ignores = cls.serialization_additional_fields()
    members = {}
    for field_name, field_value in fields.items():
        if field_name in ignores:
            continue
        field_type = _get_all_annotations(cls).get(field_name)
        if field_type is None:
            continue  # unannotated field is ignored
        custom_deserializer = None
        if field_name in overrides:
            custom_deserializer = overrides[field_name]
        if custom_deserializer is None:
            field_type = unwrap_optional(field_type)
        if isinstance(field_value, list) and typing.get_origin(field_type) == list:
            # typing.List[<type>]
            list_type = typing.get_args(field_type)[0]
            if custom_deserializer is None:
                list_type = unwrap_optional(list_type)
            member_value = [
                field_to_member(list_value, list_type, custom_deserializer)
                for list_value in field_value
            ]
        else:
            member_value = field_to_member(field_value, field_type, custom_deserializer)
        members[field_name] = member_value
    return cls(**members)


def field_to_member(field_value, field_type, custom_deserializer):
    if field_value is None or type(field_value).__name__ == 'YsonEntity':
        # YQL sometimes returns nulls as YsonEntity
        return None
    elif custom_deserializer is not None:
        return custom_deserializer(field_value)
    elif issubclass(field_type, YsonSerializable):
        return field_type.from_yson(field_value)
    elif type(field_type) == EnumMeta:
        return field_type(field_value)
    elif field_type == datetime:
        return parse_datetime(field_value)
    elif field_type == timedelta:
        return parse_timedelta(field_value)
    elif _is_primitive(field_value):
        # float values without floating point implicitly converted to int during json deserialization
        return field_type(field_value)
    else:
        raise ValueError(f'Unsupported field type: {field_type}')


def unwrap_optional(field_type) -> type:
    # unwrap <type> from typing.Optional[<type>]
    if typing.get_origin(field_type) == typing.Union:
        union_args = typing.get_args(field_type)
        if len(union_args) == 2 and union_args[1] == type(None):  # noqa
            return union_args[0]
    return field_type


# json can't contain int keys
def deserialize_dict_with_int_keys(d: typing.Dict[str, int]):
    return {int(k): v for k, v in d.items()}


# to run asserts in unittest test cases in a right way, additional json dump/load is needed because some type
# conversions during dump are not obvious (i.e. conversion non-str keys in dict to str)
def assert_serialization_is_correct(
    test_case: typing.Any,
    obj: YsonSerializable,
    yson: dict,
):
    test_case.assertEqual(yson, obj.to_yson())
    test_case.assertEqual(obj.from_yson(json.loads(json.dumps(yson))), obj)


def _is_primitive(field_value) -> bool:
    return any(isinstance(field_value, t) for t in [str, bytes, int, float, bool, dict])


def _get_all_annotations(cls):
    """
    Get all class annotations, including annotations from superclasses.

    In case of annotations collision, superclass annotations will be overriden.
    """
    result = {}
    for c in cls.mro()[::-1]:
        if hasattr(c, '__annotations__'):
            result.update(c.__annotations__)
    if len(result) == 0:
        raise AttributeError('class must have annotations for serialization')
    return result
