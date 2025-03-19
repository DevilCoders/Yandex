import random
import re

from schematics import types as schematics_types
from schematics.types import base as base_types
from schematics import exceptions as schematics_exceptions

from yc_common import models as common_models
from yc_common import validation, formatting

from . import BasePublicModel


# Requests

MAX_PAGE_SIZE = 1000  # Maximum acceptable limit value
DEFAULT_PAGE_SIZE = 100  # Default limit value
_PAGE_TOKEN_RE = re.compile(r"^[a-zA-Z0-9/+-_]{1,500}[=]{,3}$")  # Fernet token


class PageTokenType(schematics_types.StringType):
    def validate_page_token(self, value, context):
        if _PAGE_TOKEN_RE.search(value) is None:
            raise schematics_exceptions.ValidationError("Invalid page token.")


class BasePublicListingRequest(BasePublicModel):
    page_size = schematics_types.IntType(min_value=0, max_value=MAX_PAGE_SIZE, default=DEFAULT_PAGE_SIZE)
    page_token = PageTokenType()


class BasePublicSearchRequestV1Alpha1(BasePublicListingRequest):
    project_id = validation.ResourceIdType(required=True)


class BasePublicSearchRequestV1Beta1(BasePublicListingRequest):
    folder_id = validation.ResourceIdType(required=True)


class BasePublicCreateRequestV1Alpha1(BasePublicModel):
    project_id = validation.ResourceIdType(required=True)


class BasePublicCreateRequestV1Beta1(BasePublicModel):
    folder_id = validation.ResourceIdType(required=True)


class BaseBatchGetRequestV1Beta1(BasePublicModel):
    folder_id = validation.ResourceIdType()


class ZonalPublicCreateRequestV1Alpha1(BasePublicCreateRequestV1Alpha1):
    zone_id = validation.ZoneIdType(required=True)


class ZonalPublicCreateRequestV1Beta1(BasePublicCreateRequestV1Beta1):
    zone_id = validation.ZoneIdType(required=True)


# Responses


class BasePublicObjectModel(BasePublicModel):
    id = schematics_types.StringType(required=True)
    created_at = common_models.IsoTimestampType()  # This field is useless for global objects
    name = schematics_types.StringType()


class BasePublicObjectModelV1Alpha1(BasePublicObjectModel):
    project_id = schematics_types.StringType()  # This field is useless for global objects


class BasePublicObjectModelV1Beta1(BasePublicObjectModel):
    description = schematics_types.StringType()
    folder_id = schematics_types.StringType()
    labels = schematics_types.DictType(schematics_types.StringType)


class ZonalPublicObjectModelV1Alpha1(BasePublicObjectModelV1Alpha1):
    zone_id = schematics_types.StringType()


class ZonalPublicObjectModelV1Beta1(BasePublicObjectModelV1Beta1):
    zone_id = schematics_types.StringType()


class RegionalPublicObjectModelV1Beta1(BasePublicObjectModelV1Beta1):
    region_id = schematics_types.StringType()


class BaseListModel(BasePublicModel):
    next_page_token = schematics_types.StringType()


# Update Request


class FieldMaskType(base_types.StringType):
    MESSAGES = {
        "convert": "Couldn't interpret '{}' as a field mask.",
    }

    def __init__(self, *args, max_length=255, **kwargs):
        super().__init__(*args, max_length=max_length, **kwargs)

    def mock(self, context=None):
        count = random.randint(0, 5)
        part_length = self.max_length // count
        return ",".join(
            base_types.random_string(random.randrange(1, part_length))
            for _ in range(count)
        )

    def to_native(self, value, context=None):
        if isinstance(value, str):
            return set(value.split(","))
        elif isinstance(value, set):
            return value
        else:
            raise schematics_exceptions.ConversionError(self.messages["convert"].format(value))

    def to_primitive(self, value, context=None):
        return ",".join(value)


class BasePublicUpdateModel(BasePublicModel):
    update_mask = FieldMaskType(serialize_when_none=False)

    def validate_update_mask(self, data, value):
        if value is None:
            return value

        for field in value:
            if formatting.camelcase_to_underscore(field) not in self.fields:
                raise schematics_exceptions.ValidationError("Unknown field {!r}".format(field))
        return value

    def underscore_update_mask(self):
        if self.update_mask is None:
            return None
        return {formatting.camelcase_to_underscore(field) for field in self.update_mask}


def filter_by_mask(value, mask, optional_fields=None):
    """Filter dict value by mask
       Only optional_fields preserve None value
    """

    if optional_fields is None:
        optional_fields = frozenset()

    return {
        k: v for k, v in value.items()
        if k in mask and (v is not None or k in optional_fields)
    }
