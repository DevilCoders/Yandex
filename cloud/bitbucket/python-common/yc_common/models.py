"""Simple ORM for dict/json"""

import abc
import copy
import datetime
import decimal
import functools
import ipaddress
import iso8601
import random

import simplejson as json
import schematics.exceptions

from schematics.common import NOT_NONE
from schematics.datastructures import FrozenDict, FrozenList
from schematics.exceptions import ConversionError, ValidationError
from schematics.models import Model as _Model, ModelMeta
from schematics.transforms import whitelist

# Export these types to module users
# noinspection PyUnresolvedReferences
from schematics.types import BaseType, BooleanType, StringType, IntType, DateTimeType, DecimalType, FloatType, LongType, \
    NumberType, DateType, UUIDType, Serializable
from schematics.types.compound import CompoundType, ListType, DictType as _DictType, ModelType
from typing import Iterable

from yc_common.exceptions import Error
from yc_common.formatting import format_time
from yc_common.misc import doesnt_override, drop_none, network_is_subnet_of

from yc_common import logging
log = logging.get_logger(__name__)

_PUBLIC_API_ROLE = "public_api"


class ModelValidationError(Error):
    def __init__(self, message, *args, public_message="Request validation error."):
        super().__init__(message, *args)
        self.public_message = public_message


class ModelAbcMeta(abc.ABCMeta, ModelMeta):
    pass


class Model(_Model):
    @classmethod
    @doesnt_override(_Model)
    def new(cls, **kwargs):
        obj = cls()
        for attr, value in kwargs.items():
            obj[attr] = value
        return obj

    # FIXME: Should we made a deep copy here?
    @classmethod
    @doesnt_override(_Model)
    def new_from_model(cls, other_model, aliases=None):
        obj = cls()
        for target_attr, target_type in cls.fields.items():
            if aliases is not None:
                source_attr = aliases.get(target_attr, target_attr)
            else:
                source_attr = target_attr

            # Zero alias means skip
            if source_attr is None:
                continue

            source_value = other_model[source_attr]
            if source_value is None:
                continue

            obj[target_attr] = _convert_field(source_value, target_type)

        return obj

    @classmethod
    def columns(cls):
        return cls.fields.keys()

    def to_kikimr_values(self, columns=None):
        if columns is None:
            columns = self.columns()

        data = self.to_kikimr()
        values = []
        for col in columns:
            try:
                val = data.get(col)
                values.append(val)
            except KeyError:
                raise Error("{} unknown column: {!r}", self.__class__.__name__, col)

        return values

    @doesnt_override(_Model)
    def copy(self):
        """
        Returns deep copy of schematics object
        """

        # Serialization and deserialization is the only way to obtain copy of model
        cls = type(self)
        return cls(self.to_primitive())

    @staticmethod
    @doesnt_override(_Model)
    def copy_from_iterable(iterable):
        return [obj.copy() for obj in iterable]

    def __repr__(self):
        return self.__class__.__name__ + repr(self.to_native())

    @doesnt_override(_Model)
    def validate_object(self):
        """
        Validates the state of the model

        Unlike validate() this method doesn't mutate original object
        """

        try:
            self.copy().validate()
        except schematics.exceptions.BaseError as e:
            raise ModelValidationError("{} model validation error: {}.", self.__class__.__name__, e,
                                       public_message=_format_schematics_error(e))

    @doesnt_override(_Model)
    def to_db(self):
        # FIXME: Which export level to use?
        return self.to_primitive()

    @doesnt_override(_Model)
    def to_kikimr(self):
        # FIXME: Which export level to use?
        return self.to_primitive(app_data={_AppDataOptions.FORMAT: _DataFormat.KIKIMR})

    @doesnt_override(_Model)
    def to_api(self, public, internal_format=False):
        """
        Converts internal model to json

        :param public: Turn on to hide internal fields in public API
        :param internal_format: Use internal representation
        :return: json representation of object
        """

        return self.to_primitive(
            export_level=NOT_NONE, role=_PUBLIC_API_ROLE if public else None,
            app_data={_AppDataOptions.FORMAT: _DataFormat.INTERNAL} if internal_format else None)

    @doesnt_override(_Model)
    def to_user(self):
        """
        Converts model to json
        :return: json representation of object
        """

        return self.to_primitive(export_level=NOT_NONE)

    @doesnt_override(_Model)
    def to_cli(self, full):
        """Converts model to CLI representation."""

        return self.to_primitive(
            export_level=None if full else NOT_NONE, app_data={_AppDataOptions.FORMAT: _DataFormat.CLI})

    @staticmethod
    @doesnt_override(_Model)
    def to_db_from_iterable(iterable):
        return [obj.to_db() for obj in iterable]

    @staticmethod
    @doesnt_override(_Model)
    def to_api_from_iterable(iterable, public):
        return [obj.to_api(public) for obj in iterable]

    @classmethod
    @doesnt_override(_Model)
    def from_db(cls, data, validate=True, iterable=False):
        return cls.__deserialize(data, validate=validate, iterable=iterable)

    @classmethod
    @doesnt_override(_Model)
    def from_kikimr(cls, data, validate=True, iterable=False, ignore_unknown=False, partial=False):
        return cls.__deserialize(data, validate=validate, iterable=iterable, ignore_unknown=ignore_unknown, app_data={
            _AppDataOptions.FORMAT: _DataFormat.KIKIMR,
        }, partial=partial)

    @classmethod
    @doesnt_override(_Model)
    def from_api(cls, data, validate=True, ignore_unknown=False, partial=False, iterable=False):
        return cls.__deserialize(data, validate=validate, ignore_unknown=ignore_unknown, iterable=iterable, app_data={
            _AppDataOptions.FORMAT: _DataFormat.API_REQUEST,
        }, partial=partial)

    @classmethod
    def __deserialize(cls, data, validate=True, ignore_unknown=False, partial=False, iterable=False, app_data=None):
        # FIXME: We should generate more user-friendly messages for API errors + they mustn't contain class name,
        # because it may be a temporary (something like _Request).

        if iterable:
            if type(data) is not list:
                raise ModelValidationError("A list of {} models validation error: a list is expected.", cls.__name__)

            return [cls.from_api(item, validate=validate, ignore_unknown=ignore_unknown, partial=partial)
                    for item in data]
        else:
            try:
                return cls(data, validate=validate, strict=not ignore_unknown, app_data=app_data, partial=partial)
            except schematics.exceptions.BaseError as e:
                raise ModelValidationError("{} model validation error: {}.", cls.__name__, e,
                                           public_message=_format_schematics_error(e))

    @doesnt_override(_Model)
    def update(self, others: dict):
        for key, value in others.items():
            self[key] = value


def _convert_field(source_value, target_type):
    if isinstance(target_type, ListType) and isinstance(source_value, list):
        field_type = target_type.field

        if isinstance(field_type, CompoundType):
            return [_convert_field(value, field_type) for value in source_value]
        else:
            return source_value[:]
    elif isinstance(target_type, _DictType) and isinstance(source_value, dict):
        field_type = target_type.field

        if isinstance(field_type, CompoundType):
            return {key: _convert_field(value, field_type) for key, value in source_value.items()}
        else:
            return source_value.copy()
    elif isinstance(target_type, SchemalessDictType):
        return copy.deepcopy(source_value)
    elif (
        isinstance(target_type, ModelType) and target_type.model_class is not None and
        isinstance(source_value, Model)
    ):
        return target_type.model_class.new_from_model(source_value)
    elif not isinstance(target_type, CompoundType):
        return source_value

    raise Error("Unsupported type for conversion: {!r} -> {!r}.", source_value.__class__, target_type)


class DictType(_DictType):
    def __init__(self, *args, key=None, **kwargs):
        if key is not None and isinstance(key, type):
            key = key()

        self.__key_type = key

        super().__init__(*args, **kwargs)

    def _convert(self, value, context, safe=False):
        value = super()._convert(value, context=context, safe=safe)

        if self.__key_type is not None:
            value = {self.__key_type.to_native(key): field for key, field in value.items()}

        return value

    def _export(self, value, format, context):
        value = super()._export(value, format, context)

        if self.__key_type is not None:
            value = {self.__key_type.to_primitive(key): field for key, field in value.items()}

        return value


class SchemalessDictType(CompoundType):
    def _convert(self, value, context=None):
        if type(value) is dict:
            return value
        else:
            raise schematics.exceptions.ConversionError("Invalid dict value: {}.".format(value))

    def _export(self, value, format, context=None):
        return value


class JsonStr(str):
    pass


class _JsonType:
    def _convert(self, value, context):
        if (
                context.app_data.get(_AppDataOptions.FORMAT) == _DataFormat.KIKIMR and
                type(value) in (str, bytes)  # We need this check to make default=... option work
        ):
            try:
                value = json.loads(value)
            except ValueError:
                raise schematics.exceptions.ConversionError("The value is not a valid JSON.")

        return super()._convert(value, context=context)

    def _export(self, value, format, context):
        value = super()._export(value, format, context)

        if context.app_data.get(_AppDataOptions.FORMAT) == _DataFormat.KIKIMR:
            try:
                value = JsonStr(json.dumps(value))
            except ValueError:
                raise schematics.exceptions.ConversionError("The value can't be converted to JSON.")

        return value


# Special version to read data as JSON and write back in native format
class _JsonReadType:
    def _convert(self, value, context):
        if (
                context.app_data.get(_AppDataOptions.FORMAT) == _DataFormat.KIKIMR and
                type(value) in (str, bytes)  # We need this check to make default=... option work
        ):
            try:
                value = json.loads(value)
            except ValueError:
                raise schematics.exceptions.ConversionError("The value is not a valid JSON.")

        return super()._convert(value, context=context)


class JsonListType(_JsonType, ListType):
    """Represents a JSON list. Should be used only for JSON root element."""

    def __init__(self, field, **kwargs):
        super().__init__(field, **kwargs)


# Special version to read data as JSON and write back in native format
class JsonReadListType(_JsonReadType, ListType):
    """Represents a JSON list. Should be used only for JSON root element."""

    def __init__(self, field, **kwargs):
        super().__init__(field, **kwargs)


class JsonDictType(_JsonType, DictType):
    """Represents a JSON dict. Should be used only for JSON root element."""

    def __init__(self, field, **kwargs):
        super().__init__(field, **kwargs)


class JsonModelType(_JsonType, ModelType):
    """Represents a JSON object. Should be used only for JSON root element."""

    def __init__(self, model_spec, **kwargs):
        super().__init__(model_spec, **kwargs)


class JsonSchemalessDictType(_JsonType, SchemalessDictType):
    """Represents a JSON schemaless dict. Should be used only for JSON root element."""

    def __init__(self, **kwargs):
        super().__init__(**kwargs)


class EnumType(BaseType):
    """A Unicode string field."""

    allow_casts = (int, bytes)

    MESSAGES = {
        'convert': "Couldn't interpret '{0}' as enum.",
    }

    def __init__(self, enum, **kwargs):
        self.enum = enum

        super(EnumType, self).__init__(**kwargs)

    def to_primitive(self, value, context=None):
        return str(value.value)

    def to_native(self, value, context=None):
        if isinstance(value, self.enum):
            return value
        if isinstance(value, str):
            return self.enum[value]
        raise schematics.exceptions.ConversionError(self.messages['convert'].format(value))


class IsoTimestampType(BaseType):
    MESSAGES = {
        "convert": "Couldn't interpret '{}' as a timestamp.",
        "parse": "Could not parse {0}. Should be ISO 8601 or timestamp."
    }

    def mock(self, context=None):
        now = int(datetime.datetime.utcnow().timestamp())
        return random.randint(now - 1000, now + 1000)

    def to_native(self, value, context=None):
        if isinstance(value, str):
            try:
                return int(iso8601.parse_date(value).timestamp())
            except iso8601.ParseError:
                raise schematics.exceptions.ConversionError(self.messages["parse"].format(value))
        elif isinstance(value, int):
            return value
        raise schematics.exceptions.ConversionError(self.messages["convert"].format(value))

    def to_primitive(self, value, context=None):
        if context is not None:
            data_format = context.app_data.get(_AppDataOptions.FORMAT)

            if data_format == _DataFormat.CLI:
                return format_time(value)
            elif data_format == _DataFormat.KIKIMR:
                return value

        # Note: Python doesn't setup timezone info and therefore we have a bad isoformat() output
        dt = datetime.datetime.utcfromtimestamp(value)
        dt = dt.replace(tzinfo=datetime.timezone.utc)
        return dt.isoformat()


class StringEnumType(StringType):
    """All statuses and enums have public representation in uppercase and internal representation in lowercase"""

    def to_native(self, value, context=None):
        return super().to_native(value, context=context).replace("_", "-").lower()

    def to_primitive(self, value, context=None):
        result = super().to_primitive(value, context=context)

        if context is not None and context.app_data.get(_AppDataOptions.FORMAT) in (
                _DataFormat.CLI, _DataFormat.KIKIMR, _DataFormat.INTERNAL
        ):
            return result

        return result.replace("-", "_").upper()


class BytesType(StringType):
    """Bytes Type representation"""

    def to_native(self, value, context=None):
        if isinstance(value, bytes):
            return value
        raise ConversionError(self.messages['convert'].format(value))


class _IPType(BaseType):
    native_factory = None
    native_types = ()

    def __init__(self, valid_values: Iterable[ipaddress._BaseNetwork] = None, version: int = None, **kwargs):
        self.valid_values = valid_values
        self.version = version
        super().__init__(**kwargs)

    def to_native(self, value, context=None):
        if isinstance(value, self.native_types):
            return value
        else:
            try:
                return self.native_factory(value)
            except ValueError:
                raise ConversionError(self.messages["parse"].format(str(value)))

    def to_primitive(self, value, context=None):
        return str(value)

    def validate_version(self, value, context):
        if self.version is not None and value.version != self.version:
            raise ValidationError(self.messages["version"].format(value, self.version))


class IPAddressType(_IPType):
    MESSAGES = {
        "parse": "'{0}' is not a valid IP address",
        "version": "'{0}' is not a valid IPv{1} address",
    }

    native_factory = staticmethod(ipaddress.ip_address)
    native_types = (ipaddress.IPv4Address, ipaddress.IPv6Address)

    def validate_values(self, value, context):
        if self.valid_values is not None:
            for valid_value in self.valid_values:
                if value in valid_value:
                    return
            raise ValidationError("{!r} value does not belong to allowed range {!r}".format(
                str(value), ", ".join([str(c) for c in self.valid_values])
            ))

    def __eq__(self, other):
        return self.get_value() == other.get_value()

    def __hash__(self):
        return hash(self.get_value())

    def get_value(self):
        if self.owner_model and hasattr(self.owner_model, self.name):
            return getattr(self.owner_model, self.name)
        else:
            return None


class IPNetworkType(_IPType):
    MESSAGES = {
        "parse": "'{0}' is not a valid IP network",
        "version": "'{0}' is not a valid IPv{1} network",
    }

    native_factory = staticmethod(ipaddress.ip_network)
    native_types = (ipaddress.IPv4Network, ipaddress.IPv6Network)

    def validate_values(self, value, context):
        if self.valid_values is not None:
            for valid_value in self.valid_values:
                if value.version == valid_value.version and network_is_subnet_of(value, valid_value):
                    return
            raise ValidationError("{!r} value does not belong to allowed range {!r}".format(
                str(value), ", ".join([str(c) for c in self.valid_values])
            ))

    def __init__(self, max_prefix=None, min_prefix=None, **kwargs):
        self.max_prefix = max_prefix
        self.min_prefix = min_prefix
        super().__init__(**kwargs)

    def validate_prefix(self, value, context):
        # Believe it or not, but prefix length 8 is greater than 24. Network-specific stuff.
        if self.max_prefix is not None and value.prefixlen < self.max_prefix:
            raise ValidationError("{!r} value prefix length is greater than {}". format(str(value), self.max_prefix))
        if self.min_prefix is not None and value.prefixlen > self.min_prefix:
            raise ValidationError("{!r} value prefix length is less than {}". format(str(value), self.min_prefix))


# Had to move const here to avoid cycle deps
DECIMAL_PRECISION = 22
DECIMAL_SCALE = 9
DECIMAL_MAX_VALUE = int("9" * (DECIMAL_PRECISION - DECIMAL_SCALE))
DECIMAL_MIN_VALUE = -DECIMAL_MAX_VALUE
DECIMAL_CTX = decimal.Context(prec=DECIMAL_PRECISION, rounding=decimal.ROUND_HALF_UP)
DECIMAL_QUANTUM = decimal.Decimal(10) ** -DECIMAL_SCALE


def format_decimal(value: decimal.Decimal):
    """
    Formats decimal with our precision
    """
    with decimal.localcontext(DECIMAL_CTX):
        value = decimal.Decimal(value)  # Just to make sure we're using decimals
        value = value.quantize(DECIMAL_QUANTUM).normalize()
        return "{:f}".format(value)


def normalize_decimal(value: decimal.Decimal):
    """
    Normalize decimal value with our precision
    """
    if not isinstance(value, decimal.Decimal):
        if not isinstance(value, str):
            value = str(value)
        try:
            # NOTE: using Context to preserve min/max, precision and rounding strategy
            with decimal.localcontext(DECIMAL_CTX):
                value = decimal.Decimal(value).normalize()
        except (TypeError, decimal.InvalidOperation):
            raise schematics.exceptions.ConversionError("cant normalize decimal {}".format(value))
    return value


class NormalizedDecimalType(DecimalType):
    def __init__(self, **kwargs):
        super().__init__(**kwargs)
        self.min_value, self.max_value = DECIMAL_MIN_VALUE, DECIMAL_MAX_VALUE

    @staticmethod
    def to_primitive(value, context=None):
        return format_decimal(value)

    @staticmethod
    def to_native(value, context=None):
        return normalize_decimal(value)

# Model options


def get_model_options(public_api_fields):
    class Options:
        roles = {_PUBLIC_API_ROLE: whitelist(*public_api_fields)}

    return Options


class _AppDataOptions:
    FORMAT = "format"


class _DataFormat:
    CLI = "cli"
    INTERNAL = "internal"
    KIKIMR = "kikimr"
    API_REQUEST = "api-request"


# Metadata options


class MetadataOptions:
    DESCRIPTION = "description"
    QUERY_VARIABLE = "query_variable"
    INTERNAL = "internal"


def get_metadata(description=None, query_variable=None, internal=None):
    return drop_none({
        MetadataOptions.DESCRIPTION: description,
        MetadataOptions.QUERY_VARIABLE: query_variable,
        MetadataOptions.INTERNAL: internal,
    })


# Schema Serialization


_SCHEMATIC_TYPE_TO_JSON_TYPE = (
    (LongType, 'integer'),
    (IntType, 'integer'),
    (DecimalType, 'number'),
    (FloatType, 'number'),
    (NumberType, 'number'),
    (BooleanType, 'boolean'),
    (_JsonType, 'object'),
    (EnumType, 'string'),
    (DateType, 'string'),
    (DateTimeType, 'string'),
    (UUIDType, 'string'),
    (StringType, 'string'),
    (IsoTimestampType, "string"),
    (BaseType, "string"),
)


@functools.singledispatch
def jsonschema_for_type(field_class, public=False):
    for (primitive_class, primitive_type) in _SCHEMATIC_TYPE_TO_JSON_TYPE:
        if isinstance(field_class, primitive_class):
            return {
                "type": primitive_type,
            }
    else:
        raise TypeError("Unknown Field Type {}".format(field_class))


@jsonschema_for_type.register(ModelType)
def _(field_class, public=False):
    return jsonschema_for_model(field_class.model_class, public=public)


@jsonschema_for_type.register(Serializable)
def _(field_class, public=False):
    return jsonschema_for_type(field_class.type, public=public)


@jsonschema_for_type.register(ListType)
def _(field_class, public=False):
    return {
        "type": "array",
        "items": jsonschema_for_type(field_class.field, public=public)
    }


@jsonschema_for_type.register(_DictType)
def _(field_class, public=False):
    return {
        "type": "object",
        "additionalProperties": jsonschema_for_type(field_class.field, public=public)
    }


def _jsonschema_for_fields(model, public=False):
    properties = {}
    required = []

    # FIXME: improve schematics api to obtain filter
    # Note: It's possible to use hasattr/getattr with "Options" here
    filter_func = model._options.roles.get(_PUBLIC_API_ROLE if public else None)

    # Loop over each field and either evict it or convert it
    for field_name, field_instance in model.fields.items():
        # Filter out fields according specified role
        if filter_func is not None and filter_func(field_name, ""):
            continue

        # Break 3-tuple out
        serialized_name = getattr(field_instance, 'serialized_name', None) or field_name
        properties[serialized_name] = jsonschema_for_type(field_instance, public=public)

        if field_instance.metadata.get('label'):
            properties[serialized_name]['description'] = field_instance.metadata.get('label')

        if isinstance(field_instance, (DateTimeType, DateType)):
            desc = properties[serialized_name].get('description', '')
            properties[serialized_name]['description'] = desc + ' Format: "%s"' % field_instance.serialized_format

        if isinstance(field_instance, EnumType):
            properties[serialized_name]['enum'] = [value.name for value in field_instance.enum]

        if getattr(field_instance, 'required', False):
            required.append(serialized_name)

    return properties, required


def jsonschema_for_model(model, public=False):
    properties, required = _jsonschema_for_fields(model, public=public)

    schema = {
        'type': "object",
        'title': model.__qualname__,
        'properties': properties,
    }

    if required:
        schema['required'] = required

    return schema


def to_jsonschema(model):
    """Returns a representation of this schema class as a JSON schema."""
    return json.dumps(jsonschema_for_model(model))


# Error formatting


@functools.singledispatch
def _format_schematics_error(reason, field=""):
    return "Validation error"  # Hide unsupported internal errors from user


@_format_schematics_error.register(str)
def _(reason, field=""):
    if reason == "Rogue field":
        reason = "Unknown field"  # This message was filled in import_loop, we need to fix it

    if reason.endswith("."):
        reason = reason[:-1]

    if field:
        reason = "{}: {}".format(field, reason)

    return reason


@_format_schematics_error.register(schematics.exceptions.ErrorMessage)
def _(reason, field=""):
    return _format_schematics_error(reason.summary, field=field)


@_format_schematics_error.register(schematics.exceptions.BaseError)
def _(reason, field=""):
    return _format_schematics_error(reason.errors, field=field)


@_format_schematics_error.register(FrozenDict)
def _(reasons, field=""):
    if field:
        field += "."

    result = "; ".join(
        _format_schematics_error(reason, field="{}{}".format(field, sub_field))
        for sub_field, reason in reasons.items()
    )

    return _format_schematics_error(result, "")


@_format_schematics_error.register(FrozenList)
def _(reason_list, field=""):
    result = ", ".join(_format_schematics_error(reason) for reason in reason_list)
    return _format_schematics_error(result, field)
