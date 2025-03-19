"""
Field types.
"""
from typing import Any, Callable, Iterable, List, Mapping, Optional, Sequence, Union

# defusedxml monkey patching is not required as no xml parsing is involved
from xml.sax.saxutils import escape as xml_escape  # noqa
from xml.sax.saxutils import unescape as xml_unescape  # noqa

from marshmallow import fields, missing
from marshmallow.validate import Length, Range, ValidationError, Validator

from ...utils import config
from ...utils.filters_parser import FilterSyntaxError
from ...utils.filters_parser import parse as parse_filters
from ...utils.validation import NotNulStrValidator, StrValidator, geo_validator

ValidateType = Union[Optional[Callable], Iterable[Callable]]


class IntBoolean(fields.Boolean):
    """
    Boolean that returns 0/1 instead of False/True.
    """

    def _deserialize(self, value, attr, data):
        return 1 if super()._deserialize(value, attr, data) else 0


class UInt(fields.Int):
    """
    Unsigned integer.
    """

    def __init__(self, validate: ValidateType = None, **kwargs) -> None:
        super().__init__(validate=_combine_validate(validate, Range(min=0)), **kwargs)


class Str(fields.Str):
    """
    PostgreSQL-friendly string

    \0's are forbidden
    """

    def __init__(self, validate: ValidateType = None, **kwargs) -> None:
        super().__init__(validate=_combine_validate(validate, NotNulStrValidator()), **kwargs)


class FolderId(Str):
    """
    Type of folderId fields.
    """

    def __init__(self, **kwargs) -> None:
        super().__init__(attribute='folder_id', description='Folder ID.', **kwargs)


class CloudId(Str):
    """
    Type of cloudId fields.
    """

    def __init__(self, **kwargs) -> None:
        super().__init__(attribute='cloud_id', description='Cloud ID.', **kwargs)


class ClusterId(Str):
    """
    Type of clusterId fields.
    """

    def __init__(self, **kwargs) -> None:
        super().__init__(attribute='cluster_id', description='Cluster ID.', **kwargs)


class ZoneId(Str):
    """
    Type of zoneId fields.
    """

    def __init__(self, validate: ValidateType = None, **kwargs) -> None:
        super().__init__(
            attribute='zone_id',
            validate=_combine_validate(validate, geo_validator),
            description='ID of availability zone.',
            **kwargs
        )


class Description(Str):
    """
    Type for description field.
    """

    def __init__(self, validate: ValidateType = None, **kwargs) -> None:
        super().__init__(
            description='Description.', validate=_combine_validate(validate, Length(min=0, max=256)), **kwargs
        )


LABELS_INVALID_CHARS = r'[^-_./\\@0-9a-z]+'


class Labels(fields.Dict):
    """
    Type for labels fields.

    - Each resource can have multiple labels, up to a maximum of 64.
    - Keys have a minimum length of 1 character and
      a maximum length of 63 characters, and cannot be empty.
    - Values can be empty, and have a maximum length of 63 characters.
    - Keys and values can contain only lowercase letters,
      numeric characters, underscores, dashes, dot, slash, backslash, and '@'.
      All characters must use ASCII encoding (International characters not allowed).
    - The key portion of a label must be unique.
      However, you can use the same key with multiple resources.
    - Keys must start with a lowercase letter or international character.

    """

    _key_validator = StrValidator('key', min_size=1, max_size=63, invalid_chars=LABELS_INVALID_CHARS)
    _value_validator = StrValidator('value', min_size=0, max_size=63, invalid_chars=LABELS_INVALID_CHARS)

    def __init__(self, **kwargs) -> None:
        super().__init__(description='Labels.', validate=Length(min=0, max=64), **kwargs)

    @staticmethod
    def _validate_label_key_first_char(value: str) -> None:
        first_char = value[0]
        if first_char.isalpha() and first_char.islower():
            return
        raise ValidationError('Keys must start with a lowercase letter.')

    def _deserialize(self, value, attr, data):
        validated_dict = super()._deserialize(value, attr, data)
        # Current marshmallow, don't support dictionary
        # keys and values validation
        for label_key, label_value in validated_dict.items():
            self._key_validator(label_key)
            self._validate_label_key_first_char(label_key)
            self._value_validator(label_value)
        return validated_dict


class Filter(Str):
    """
    Filter field.

    Verify filter string length and parse filter query.
    """

    def __init__(self, **kwargs) -> None:
        super().__init__(description='Filter.', attribute='filters', **kwargs)

    def _deserialize(self, value, attr, data):
        validate_str = super()._deserialize(value, attr, data)
        # validate string length by-hand,
        # cause marshmallow.validate happens after deserialize
        Length(min=1, max=1000)(validate_str)
        try:
            return parse_filters(validate_str)
        except FilterSyntaxError as exc:
            raise ValidationError(str(exc))


class EnumChoiceValidator(Validator):
    """
    This is simple "non-validating" validator.
    It has choices property as OneOf validator (this is used in openapi spec generation).
    """

    def __init__(self, field):
        self._field = field

    def __call__(self, value):
        return value

    @property
    def choices(self):
        """
        Return choices for field2choices renderer
        """
        return self._field.mapping.keys()


MappingType = Mapping[str, Any]


class MappedEnum(Str):
    """
    Enumeration field with different values in serialized and deserialized
    representations.
    """

    default_error_messages = {
        'invalid_enum_value': 'Invalid value \'{value}\', allowed values: {allowed_values}',
    }

    def __init__(self, mapping: Union[MappingType, Callable[[], MappingType]], **kwargs) -> None:
        super().__init__(**kwargs)
        self._mapping_source = mapping
        self.validators = [EnumChoiceValidator(self)]

    @property
    def mapping(self) -> MappingType:
        """
        Return map from serialized to deserialized representation
        """
        if isinstance(self._mapping_source, Mapping):
            if 'description' not in self.metadata and 'skip_description' not in self.metadata:
                self.metadata['description'] = self._mapping_source
            return self._mapping_source

        if 'description' not in self.metadata and 'skip_description' not in self.metadata:
            self.metadata['description'] = self._mapping_source()
        return self._mapping_source()

    def _serialize(self, value, attr, obj):
        for serialized_value, deserialized_value in self.mapping.items():
            if value == deserialized_value:
                return serialized_value
            if isinstance(deserialized_value, list):
                for dvalue in deserialized_value:
                    if value == dvalue:
                        return serialized_value

        self._fail(value, self.mapping.values())

    def _deserialize(self, value, attr, data):
        validated_str = super()._deserialize(value, attr, data)
        if validated_str in self.mapping:
            return self.mapping[validated_str]

        self._fail(validated_str, self.mapping.keys())

    def _fail(self, invalid_value, allowed_values):
        self.fail('invalid_enum_value', value=invalid_value, allowed_values=', '.join(sorted(map(str, allowed_values))))


class UpperToLowerCaseEnum(MappedEnum):
    """
    Specialization of MappedEnum where deserialized field values equal to
    lower cased values of serialized field representation.
    """

    def __init__(self, allowed_values: Sequence[str], **kwargs) -> None:
        super().__init__({value: value.lower() for value in allowed_values}, **kwargs)


class PrefixedUpperToLowerCaseEnum(MappedEnum):
    """
    Specialization of MappedEnum where deserialized field values equal to
    lower cased values of serialized field representation without prefix.
    """

    def __init__(self, allowed_values: Sequence[str], prefix: str, **kwargs) -> None:
        super().__init__({value: value.lower().split(prefix.lower())[1] for value in allowed_values}, **kwargs)


class PrefixedUpperToLowerCaseAndSpacesEnum(MappedEnum):
    """
    Specialization of MappedEnum where deserialized field values equal to
    lower cased values of serialized field representation without prefix.
    """

    def __init__(self, allowed_values: Sequence[str], prefix: str, **kwargs) -> None:
        super().__init__(
            {value: value.lower().split(prefix.lower())[1].replace('_', ' ') for value in allowed_values}, **kwargs
        )


class UnderscoreToSpaceCaseEnum(MappedEnum):
    """
    Replace underscore (_) in MappedEnum with space ( ).
    """

    def __init__(self, allowed_values: Sequence[str], **kwargs) -> None:
        super().__init__({value: value.replace('_', ' ') for value in allowed_values}, **kwargs)


class UnderscoreToHyphenCaseEnum(MappedEnum):
    """
    Replace underscore (_) in MappedEnum with hyphen (aka minus) (-).
    """

    def __init__(self, allowed_values: Sequence[str], **kwargs) -> None:
        super().__init__({value: value.replace('_', '-') for value in allowed_values}, **kwargs)


class Environment(MappedEnum):
    """
    Type of environment fields.
    """

    def __init__(self, mapping=config.environment_mapping, **kwargs):
        super().__init__(
            lambda: {v.upper(): k for k, v in mapping().items()},
            error_messages={
                'invalid_enum_value': 'Invalid environment \'{value}\', allowed values: {allowed_values}',
            },
            **kwargs,
            skip_description=True
        )


class XmlEscapedStr(Str):
    """
    String which deserialized representation is XML escaped.
    """

    def _serialize(self, value, attr, obj):
        str_value = super()._serialize(value, attr, obj)
        if str_value is None:
            return None

        return xml_unescape(str_value)

    def _deserialize(self, value, attr, data):
        return xml_escape(super()._deserialize(value, attr, data))


class GrpcInt(fields.Int):
    """
    Integer with gRPC semantic where "0" is interpreted as no value.
    """

    def _deserialize(self, value, attr, data):
        if value in (0, '0'):
            if self.missing is missing:
                return missing
            value = self.missing

        return super()._deserialize(value, attr, data)

    def _validate(self, value):
        if value is not missing:
            super()._validate(value)


class GrpcUInt(GrpcInt):
    """
    Unsigned integer with gRPC semantic where "0" is interpreted as no value.
    """

    def __init__(self, validate: ValidateType = None, **kwargs) -> None:
        super().__init__(validate=_combine_validate(validate, Range(min=0)), **kwargs)


class GrpcStr(Str):
    """
    String with gRPC semantic where "" is interpreted as no value.
    """

    def __init__(self, validate: ValidateType = None, **kwargs) -> None:
        super().__init__(allow_none=True, **kwargs)

    def _deserialize(self, value, attr, data):
        if value == '':
            return None
        return super()._deserialize(value, attr, data)


class MillisecondsMappedToSeconds(fields.Int):
    """
    Duration in milliseconds with deserialized representation in seconds.
    """

    default_error_messages = {
        'invalid_duration': 'Value must be a multiple of 1000 (1 second)',
    }

    def _serialize(self, value, attr, obj):
        value = value * 1000 if value is not None else None
        return super()._serialize(value, attr, obj)

    def _deserialize(self, value, attr, data):
        deserialized = super()._deserialize(value, attr, data)
        if deserialized is None:
            return None

        if deserialized % 1000 != 0:
            self.fail('invalid_duration')

        return int(deserialized / 1000)

    def _validate(self, value):
        super()._validate(value * 1000)


class BooleanMappedToInt(fields.Boolean):
    """
    Boolean mapped to integer in deserialized representation.
    """

    def _serialize(self, value, attr, obj):
        bool_value = bool(value) if value is not None else None
        return super()._serialize(bool_value, attr, obj)

    def _deserialize(self, value, attr, data):
        deserialized = super()._deserialize(value, attr, data)
        if deserialized is None:
            return None

        return 1 if deserialized else 0


def _combine_validate(source_validate: ValidateType, extra_validate: Callable) -> List[Callable]:
    result = [extra_validate]

    if source_validate is None:
        return result

    if callable(source_validate):
        result.append(source_validate)
    else:
        result.extend(source_validate)

    return result
