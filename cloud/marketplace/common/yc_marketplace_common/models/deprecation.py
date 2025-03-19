from typing import Optional

from cloud.marketplace.common.yc_marketplace_common.models.abstract_mkt_base import MktBasePublicModel
from yc_common.misc import timestamp
from yc_common.models import IsoTimestampType
from yc_common.models import StringEnumType
from yc_common.models import StringType
from yc_common.models import get_model_options


class Deprecation(MktBasePublicModel):
    class Status:
        DEPRECATED = "deprecated"
        OBSOLETE = "obsolete"
        DELETED = "deleted"
        ALL = {DEPRECATED, OBSOLETE, DELETED}

        Transitions = {
            None: {DEPRECATED},
            DEPRECATED: {DEPRECATED, OBSOLETE},
            OBSOLETE: {OBSOLETE, DELETED},
            DELETED: {DELETED},
        }

    status = StringEnumType(choices=Status.ALL)
    description = StringType()
    deprecated_at = IsoTimestampType(required=True)
    replacement_uri = StringType()

    Options = get_model_options(public_api_fields=(
        "status",
        "description",
        "deprecated_at",
        "replacement_uri",
    ))

    @classmethod
    def new(cls, description="", replacement_uri="", deprecated_at=timestamp(), status=None, **kwargs) -> "Deprecation":
        return super().new(
            description=description,
            replacement_uri=replacement_uri,
            status=Deprecation.Status.DEPRECATED if status is None else status,
            deprecated_at=deprecated_at,
            **kwargs,
        )

    @classmethod
    def create(cls, description="", replacement_uri="", deprecated_at=timestamp(), **kwargs) -> "Deprecation":
        return super().new(
            description=description,
            replacement_uri=replacement_uri,
            status=Deprecation.Status.DEPRECATED,
            deprecated_at=deprecated_at,
            **kwargs,
        )

    @classmethod
    def to_stage(cls, deprecation: Optional["Deprecation"]) -> int:
        if deprecation is None:
            return 0

        status = deprecation.status
        if status == Deprecation.Status.DEPRECATED:
            return 1
        elif status == Deprecation.Status.OBSOLETE:
            return 2
        else:
            return 3
