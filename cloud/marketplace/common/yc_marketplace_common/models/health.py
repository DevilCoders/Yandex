from schematics.types import BooleanType

from yc_common.models import Model
from yc_common.models import SchemalessDictType
from yc_common.models import StringEnumType
from yc_common.models import get_model_options


class HealthCheck(Model):
    health = StringEnumType()
    details = SchemalessDictType()

    Options = get_model_options(
        public_api_fields=(
            "health",
            "details",
        ),
    )


class HealthModel(Model):
    monrun = BooleanType(metadata={"description": "Set the monrun output format"})
