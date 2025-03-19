import os
import pathlib

import dataclasses

from cloud.dwh.nirvana.config import BaseDeployConfig as _BaseDeployConfig

NIRVANA_QUOTA = 'cloud-dwh'

YT_CLUSTER = 'hahn'
MR_CLUSTER = YT_CLUSTER

YT_TOKEN = 'robot-ycloud-dwh__yt-token'
YT_OWNERS = 'robot-ycloud-dwh,apastron,domitrokhin,kbespalov,kuchits-ya,olevino999,remezova'
TTL = 120

DBAAS_TOKEN = 'robot_clanalytic-mdb'
ST_TOKEN = 'robot-clanalytics-startrek'
YQL_TOKEN = 'robot-ycloud-dwh__yql-token'
STAFF_API_TOKEN = 'robot-ycloud-dwh__staff-api-token'

MDB_TOKEN = 'robot-ycloud-dwh__mdb-oauth-token'

BILLING_CLICKHOUSE_CLUSTER_ID = 'mdb4rd721uh94u42qlf1'
BILLING_CLICKHOUSE_USER = 'robot-ycloud-dwh'
BILLING_CLICKHOUSE_PASSWORD = 'robot-ycloud-dwh__clickhouse-password'

SOLOMON_TOKEN = 'robot-ycloud-dwh__solomon-token'
SOLOMON_CLUSTER = 'yc-dwh'

_ARC_ROOT = os.environ.get("ARC_ROOT") or os.path.expanduser("~/arc/arcadia")

SQL_BASE_DIR_PG = pathlib.Path(_ARC_ROOT) / "cloud" / "dwh" / "pg" / "dml"

SPYT_DISCOVERY_DIR = '//home/cloud_analytics/spark-discovery/spark3'
SPYT_YT_PROXY = 'hahn'
SPYT_JOBS_YT_ROOT = 'yt:///home/cloud_analytics/dwh/spark/jobs'
SPYT_JARS_YT_ROOT = 'yt:///home/cloud_analytics/spark/jars'

SPARK_DEFAULT_LOCALE_CONFIG = [
    "spark.driver.extraJavaOptions=-Duser.timezone=UTC",
    "spark.executor.extraJavaOptions=-Duser.timezone=UTC",
    "spark.sql.session.timeZone=UTC",
]

SPARK_DEFAULT_PYTHON_CONFIG = [
    "spark.pyspark.python=\"/opt/python3.7/bin/python3.7\""
]

SPARK_DEFAULT_CONFIG = SPARK_DEFAULT_LOCALE_CONFIG + [
    "spark.executor.instances=2",
    "spark.executor.memory=2G",
    "spark.executor.cores=2",
    "spark.cores.max=5"
]

SPARK_DEFAULT_PY_FILES = ["yt:///home/cloud_analytics/dwh/spark/dependencies.zip"]

YAV_OAUTH_SECRET_NAME = "robot-clanalytics-yav"
YT_OAUTH_SECRET_NAME = 'robot-clanalytics-yt'

REACTOR_PREFIX = "/cloud/dwh"
REACTOR_OWNER = 'robot-ycloud-dwh'

PG_USER = "etl-user"
PG_DATABASE = "cloud_dwh"
PG_HOST = "c-mdb90eeq06kd7g5oe3et.rw.db.yandex.net"
PG_PORT = "6432"
PG_PASSWORD_SECRET_NAME = "cloud-dwh-pg-etl-password-prod"
MDB_OAUTH_SECRET_NAME = "robot_clanalytic-mdb"


@dataclasses.dataclass(frozen=True)
class BaseDeployConfig(_BaseDeployConfig):
    environment: str = None

    yt_cluster: str = dataclasses.field(default=YT_CLUSTER)
    yt_token: str = dataclasses.field(default=YT_TOKEN)
    yt_owners: str = dataclasses.field(default=YT_OWNERS)
    ttl: int = dataclasses.field(default=TTL)

    yt_folder: str = dataclasses.field(default=None)

    yql_token: str = dataclasses.field(default=YQL_TOKEN)

    solomon_token: str = dataclasses.field(default=SOLOMON_TOKEN)
    solomon_cluster: str = dataclasses.field(default=SOLOMON_CLUSTER)

    nirvana_quota: str = dataclasses.field(default=NIRVANA_QUOTA)

    mdb_token: str = dataclasses.field(default=MDB_TOKEN)

    staff_token: str = dataclasses.field(default=STAFF_API_TOKEN)

    reactor_prefix: str = dataclasses.field(default=REACTOR_PREFIX)
    reactor_owner: str = dataclasses.field(default=REACTOR_OWNER)

    def __typing__(self):
        """
        Just for syntax highlighting
        """
        self.mr_account: str = ''
        self.yt_pool: str = ''
        self.yt_tmp_path: str = ''

    @property
    def yt_tmp_layer_path(self):
        return os.path.join(self.yt_folder, 'tmp')

    @property
    def yt_query_cache_path(self):
        return os.path.join(self.yt_tmp_layer_path, 'query_cache')
