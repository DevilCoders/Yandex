import dataclasses
from typing import Tuple

from cloud.analytics.nirvana.vh.config.base import BaseDeployConfig

ENVIRONMENT = 'production'

YT_TMP_PATH = f'//home/cloud_analytics/tmp/vh/{ENVIRONMENT}'
YT_POOL = 'cloud_analytics_pool'

WORKFLOW_GROUP_NS_ID = 9072323  # https://nirvana.yandex-team.ru/browse?selected=9072323


@dataclasses.dataclass(frozen=True)
class DeployConfig(BaseDeployConfig):
    nirvana_workflow_group_ns_id: int = dataclasses.field(default=WORKFLOW_GROUP_NS_ID)
    nirvana_tags: Tuple[str] = ('yc_analytics', ENVIRONMENT)

    yt_pool: str = dataclasses.field(default=YT_POOL)
    yt_tmp_path: str = dataclasses.field(default=YT_TMP_PATH)
