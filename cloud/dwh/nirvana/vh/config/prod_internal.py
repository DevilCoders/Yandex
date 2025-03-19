from typing import Tuple

import dataclasses

from cloud.dwh.nirvana.vh.config.base import BaseDeployConfig

ENVIRONMENT = 'prod_internal'

MR_ACCOUNT = 'cloud-dwh'

YT_FOLDER = '//home/cloud-dwh/data/prod_internal'
YT_POOL = 'cloud-dwh'
TTL = 120

WORKFLOW_GROUP_NS_ID = 10010788  # https://nirvana.yandex-team.ru/browse?selected=10010788


@dataclasses.dataclass(frozen=True)
class DeployConfig(BaseDeployConfig):
    nirvana_workflow_group_ns_id: int = dataclasses.field(default=WORKFLOW_GROUP_NS_ID)
    nirvana_tags: Tuple[str] = ('yc_dwh', ENVIRONMENT)

    mr_account: str = dataclasses.field(default=MR_ACCOUNT)

    yt_pool: str = dataclasses.field(default=YT_POOL)
    yt_folder: str = dataclasses.field(default=YT_FOLDER)
    ttl: int = dataclasses.field(default=TTL)

    solomon_cluster: str = dataclasses.field(default=ENVIRONMENT)
