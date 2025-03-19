import dataclasses

from cloud.dwh.nirvana.config import BaseDeployConfig as _BaseDeployConfig

NIRVANA_QUOTA = 'coud-analytics'

YT_CLUSTER = 'hahn'
MR_CLUSTER = YT_CLUSTER
YT_TOKEN = 'robot-clanalytics-yt'
YT_OWNERS = 'robot-clanalytics-yt,makhalin,kbespalov'
MR_ACCOUNT = 'cloud_analytics'

YQL_TOKEN = 'robot-clanalytics-yql'

REACTOR_PREFIX = "/cloud/analytics"
REACTOR_OWNER = 'robot-clanalytics-yt'

YQL_BASE_DIR = '../../../../yql'


@dataclasses.dataclass(frozen=True)
class BaseDeployConfig(_BaseDeployConfig):
    environment: str = None

    yt_cluster: str = dataclasses.field(default=YT_CLUSTER)
    yt_token: str = dataclasses.field(default=YT_TOKEN)
    yt_owners: str = dataclasses.field(default=YT_OWNERS)

    mr_account: str = dataclasses.field(default=MR_ACCOUNT)

    yql_token: str = dataclasses.field(default=YQL_TOKEN)

    nirvana_quota: str = dataclasses.field(default=NIRVANA_QUOTA)

    reactor_prefix: str = dataclasses.field(default=REACTOR_PREFIX)
    reactor_owner: str = dataclasses.field(default=REACTOR_OWNER)

    def __typing__(self):
        """
        Just for syntax highlighting
        """
        self.yt_pool: str = ""
        self.yt_tmp_path: str = ""
