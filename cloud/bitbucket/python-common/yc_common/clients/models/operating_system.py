from yc_common import models as common_models
from yc_common.clients.models import base as base_models


class OperatingSystem(base_models.BasePublicModel):
    class Type:
        LINUX = "linux"
        WINDOWS = "windows"

        ALL = [LINUX, WINDOWS]

    type = common_models.StringEnumType()
