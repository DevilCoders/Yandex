from schematics.exceptions import ConversionError
from schematics.exceptions import ValidationError
from schematics.types import DictType
from schematics.types import ListType

from cloud.marketplace.common.yc_marketplace_common.models.abstract_mkt_base import MktBasePublicModel
from cloud.marketplace.common.yc_marketplace_common.utils.db import DataModel
from cloud.marketplace.common.yc_marketplace_common.utils.db import Table
from yc_common.clients.kikimr.client import KikimrDataType
from yc_common.clients.kikimr.client import KikimrTableSpec
from yc_common.models import IsoTimestampType
from yc_common.models import Model
from yc_common.models import StringType


class _Lang:
    EN = "en"
    RU = "ru"

    ALL = {EN, RU}
    FALLBACK_LINE = [EN, RU]


class I18n(Model):
    LANGUAGES = _Lang.ALL
    FALLBACK_LINE = _Lang.FALLBACK_LINE
    PREFIX = "mkt_i18n://"

    id = StringType(required=True)  # special generated id
    created_at = IsoTimestampType(required=True)

    lang = StringType(required=True, choices=_Lang.ALL)
    text = StringType(required=True)

    data_model = DataModel((
        Table(name="i18n", spec=KikimrTableSpec(
            columns={
                "id": KikimrDataType.UTF8,
                "created_at": KikimrDataType.UINT64,
                "lang": KikimrDataType.UTF8,
                "text": KikimrDataType.UTF8,
            },
            primary_keys=["id", "lang"],
        )),
    ))

    @classmethod
    def db_fields(cls, table_name="") -> str:
        if table_name:
            table_name += "."

        return ",".join("{}{}".format(table_name, key) for key, _ in cls.fields.items())


class LangType(DictType):
    def __init__(self, **kwargs):
        super().__init__(StringType, coerce_key=None, **kwargs)

    @staticmethod
    def _lang_validate(value: dict) -> bool:
        return len(value.keys() - _Lang.ALL - {"_id"}) == 0

    def _convert(self, value: dict, context, safe=False):
        if not self._lang_validate(value):
            raise ConversionError("Not implemented language!")
        return super()._convert(value, context, safe)

    def validate_key(self, value, context):

        if not self._lang_validate(value):
            raise ValidationError("Not implemented language!")


class BulkI18nCreatePublic(MktBasePublicModel):
    translations = ListType(LangType(), required=True)


class I18nGetRequest(MktBasePublicModel):
    id = StringType(required=True)
