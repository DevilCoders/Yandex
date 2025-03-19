"""JSON validation tools."""

import re
from typing import Set
import unicodedata
from urllib import parse as urllib_parse

import regex

from schematics import exceptions as schematics_exceptions
from schematics import types as schematics_types
from schematics.types.net import URI_PATTERNS as SCHEMATICS_URI_PATTERNS

from yc_common.logging import get_logger
from yc_common.exceptions import (
    Error,
    InvalidResourceIdError,
    InvalidResourceNameError,
    RequestValidationError,
)
from yc_common.formatting import camelcase_to_underscore

log = get_logger(__name__)


class RegionId:
    RU_CENTRAL1 = "ru-central1"

    ALL = [RU_CENTRAL1]


class SchemaValidationError(Error):
    pass


# FIXME: Allow only one format
# Note: Check that no code use these limitations before change (e.g. name should start with [a-z] only)
_ID_RE = re.compile(r"^([0-9a-f]{8}-[0-9a-f]{4}-[0-9a-f]{4}-[0-9a-f]{4}-[0-9a-f]{12}|[0-9a-f]{32})$")
_NEW_ID_RE = re.compile(r"^[a-z][-a-z0-9]{,18}[a-z0-9]$")
_NEW_ID_PREFIX_RE = re.compile(r"^[a-z][a-z0-9]{2}$")
_NAME_RE = re.compile(r"^[a-z]([-a-z0-9]{,61}[a-z0-9])?$")


def validate_uuid(uuid, message, *message_args):
    uuid = uuid.lower()

    if _ID_RE.search(uuid) is None:
        raise RequestValidationError(message, *message_args)

    return uuid


def validate_resource_id(resource_id):
    if _ID_RE.search(resource_id) is None and _NEW_ID_RE.search(resource_id) is None:
        raise InvalidResourceIdError()

    return resource_id


def validate_resource_name(resource_name):
    if _NAME_RE.search(resource_name) is None:
        raise InvalidResourceNameError()

    return resource_name


def validate_id_prefix(prefix):
    if _NEW_ID_PREFIX_RE.search(prefix) is None:
        raise RequestValidationError("Invalid ID prefix {!r}.", prefix)

    return prefix


# schematics compatible validators


_DOMAIN_PART_RE = r"(?:[0-9a-z]|[0-9a-z][0-9a-z-]*[0-9a-z])"
_HOSTNAME_RE = re.compile(r"^(?:" + _DOMAIN_PART_RE + r"\.)*" + _DOMAIN_PART_RE + "$")
_ZONE_ID_RE = re.compile("^" + _DOMAIN_PART_RE + "$")


class ResourceNameType(schematics_types.StringType):
    def __init__(self, *args, **kwargs):
        super().__init__(*args, max_length=256, **kwargs)

    def validate_resource_name(self, value, context):
        if _NAME_RE.search(value) is None:
            raise schematics_exceptions.ValidationError("Invalid resource name.")


class ResourceIdType(schematics_types.StringType):
    def validate_resource_id(self, value, context):
        if _ID_RE.search(value) is None and _NEW_ID_RE.search(value) is None:
            raise schematics_exceptions.ValidationError("Invalid resource ID.")


# Attention: Be careful here: we use `regex` module instead of `re` module here because it supports Unicode
# categories (http://www.regular-expressions.info/unicode.html).
_WHITESPACES_RE = regex.compile(r"[\p{Separator}\p{Other}]+", re.UNICODE)
_INVALID_SYMBOLS_RE = regex.compile(r"[^\w \p{Letter}\p{Number}\p{Mark}\p{Punctuation}\p{Symbol}]+", re.UNICODE)


class ResourceDescriptionType(schematics_types.StringType):
    MESSAGES = {
        "convert": "Invalid characters: '{!s}'.",
    }

    def __init__(self, **kwargs):
        super().__init__(max_length=256, **kwargs)

    def to_native(self, value, context=None):
        # See https://stackoverflow.com/questions/16467479/normalizing-unicode
        value = unicodedata.normalize("NFC", value)

        # Replace all whitespace and control characters with regular space symbol
        value = _WHITESPACES_RE.sub(" ", value).strip()

        match = _INVALID_SYMBOLS_RE.search(value)
        if match is not None:
            raise schematics_exceptions.ConversionError(self.messages["convert"].format(match.group(0)))

        return value


class HostnameType(schematics_types.StringType):
    def __init__(self, *args, **kwargs):
        super().__init__(*args, max_length=256, **kwargs)

    def validate_hostname(self, value, context):
        if _HOSTNAME_RE.search(value) is None:
            raise schematics_exceptions.ValidationError("Invalid hostname.")


class RegionIdType(schematics_types.StringType):
    def __init__(self, *args, **kwargs):
        super().__init__(*args, max_length=256, **kwargs)

    def validate_region_id(self, value, context):
        if value not in RegionId.ALL:
            raise schematics_exceptions.ValidationError("Invalid region identifier.")


class ZoneIdType(schematics_types.StringType):
    def __init__(self, *args, **kwargs):
        super().__init__(*args, max_length=256, **kwargs)

    def validate_zone_id(self, value, context):
        if _ZONE_ID_RE.search(value) is None:
            raise schematics_exceptions.ValidationError("Invalid zone identifier.")


class IdPrefixType(schematics_types.StringType):
    def validate_id_prefix(self, value, context):
        if _NEW_ID_PREFIX_RE.search(value) is None:
            raise schematics_exceptions.ValidationError("Invalid ID prefix.")


class UIntType(schematics_types.IntType):
    def __init__(self, *args, **kwargs):
        super().__init__(*args, min_value=0, **kwargs)


class LBPortType(schematics_types.IntType):
    MIN_PORT = 1
    MAX_PORT = 65535

    RANGE = [MIN_PORT, MAX_PORT]

    def __init__(self, *args, **kwargs):
        super().__init__(*args, min_value=self.MIN_PORT, max_value=self.MAX_PORT, **kwargs)


class UrlQueryPathType(schematics_types.StringType):
    """A field that stores a valid HTTP query path."""

    QUERY_PATH_REGEX = re.compile(r"""^(
            (?P<path> / %(path)s   )
        (\? (?P<query>  %(query)s  )     )?
        (\# (?P<frag>   %(frag)s   )     )?)$
        """ % SCHEMATICS_URI_PATTERNS, re.I + re.X)

    def validate_path(self, value, context=None):
        if self.QUERY_PATH_REGEX.search(value) is None:
            raise schematics_exceptions.ValidationError("Invalid URL query path")

    def to_native(self, value, context=None):
        url = urllib_parse.urlparse(value)
        qs = urllib_parse.parse_qsl(url.query)
        if qs:
            qs = urllib_parse.urlencode(qs)
            return "{}?{}".format(url.path, qs)
        else:
            return url.path


_FIELD_NAME_RAW_RE = r"[a-z]([_a-zA-Z0-9])*(\.[a-z]([_a-zA-Z0-9])*)*"
_FIELD_MASK_RE = re.compile(r"^" + _FIELD_NAME_RAW_RE + r"(," + _FIELD_NAME_RAW_RE + r")*$")


def validate_field_mask(fields, value):
    if _FIELD_MASK_RE.search(value) is None:
        raise schematics_exceptions.ValidationError("Invalid field mask.")

    if fields is None:
        return

    for user_field_name in value.split(","):
        field_name = camelcase_to_underscore(user_field_name.strip())
        if field_name not in fields:
            raise schematics_exceptions.ValidationError(
                "Invalid field mask: invalid field name {!r}.".format(user_field_name))


class FieldMaskType(schematics_types.StringType):
    def __init__(self, *args, **kwargs):
        self._fields = kwargs.pop("fields")
        super().__init__(*args, **kwargs)

    def validate_field_mask(self, value, context):
        validate_field_mask(self._fields, value)


def field_mask_to_set(field_mask) -> Set[str]:
    fields = set()
    if field_mask is None:
        return fields

    for user_field_name in field_mask.split(","):
        fields.add(camelcase_to_underscore(user_field_name.strip()))

    return fields


def get_fields_to_update(request, field_mask_spec: FieldMaskType):
    update = {}
    field_names = field_mask_spec._fields

    if request.update_mask:
        for field_name in field_mask_to_set(request.update_mask):
            update[field_name] = getattr(request, field_name)
    else:
        for field_name in field_names:
            value = getattr(request, field_name)
            if value is not None:
                update[field_name] = value

    return update
