from typing import Tuple

import dataclasses

from cloud.dwh.nirvana.vh.config.base import BaseDeployConfig

ENVIRONMENT = 'uat'

YT_POOL = None
TTL = 120

WORKFLOW_GROUP_NS_ID = 9078831  # https://nirvana.yandex-team.ru/browse?selected=9078831


@dataclasses.dataclass(frozen=True)
class DeployConfig(BaseDeployConfig):
    nirvana_workflow_group_ns_id: int = dataclasses.field(default=WORKFLOW_GROUP_NS_ID)
    nirvana_tags: Tuple[str] = ('yc_dwh', ENVIRONMENT)

    yt_pool: str = dataclasses.field(default=YT_POOL)
    ttl: int = dataclasses.field(default=TTL)
