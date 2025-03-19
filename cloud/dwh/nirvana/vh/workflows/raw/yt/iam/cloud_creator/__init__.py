import vh

from cloud.dwh.nirvana.vh.common.operations import PYSPYT
from cloud.dwh.nirvana.vh.common.operations import get_default_spark_op_pg_1secret
from cloud.dwh.nirvana.vh.config.base import SPYT_JOBS_YT_ROOT
from cloud.dwh.nirvana.vh.config.base import YT_TOKEN
from cloud.dwh.nirvana.vh.config.base import BaseDeployConfig as DeployConfig
from cloud.dwh.nirvana.config import DeployContext

WORKFLOW_LABEL = "REST IAM cloud_creator to YT to PG"
WORKFLOW_SCHEDULE = "0 0 * * * ? *"
SOURCE_SYSTEM_NAME = "iam"


def main(ctx: DeployConfig):
    CLOUD_OWNERS_FOLDER = "//home/cloud_analytics/import/iam/cloud_owners/1h"
    CLOUD_OWNERS_LATEST_FOLDER = CLOUD_OWNERS_FOLDER + "/latest"

    with vh.Graph() as g:
        op_cloud_creators_to_yt = vh.op(id="e4d3b392-715c-410e-86d7-edbf4c82be5c")
        mr_table = op_cloud_creators_to_yt(
            _name="IAM Cloud Creators -> YT (sandbox task)",
            sandbox_oauth_token="robot-clanalytics-sandbox",
            owner="CLOUD_ANALYTICS",
            cloud_creators_requests_delay=0,
            cloud_owners_dst_yt_prefix=CLOUD_OWNERS_FOLDER,
            dst_cluster="hahn",
            iam_api_endpoint="https://identity.private-api.cloud.yandex.net:14336/v1",
            juggler_host="cloud-analytics-scheduled-jobs",
            yt_token_name="robot-clanalytics-yt-yt-token"
        )

        op_mr_copy_table = vh.op(id="23762895-cf87-11e6-9372-6480993f8e34")
        op_mr_copy_table(
            _name="MRTable update latest",
            src=mr_table,
            dst_cluster="hahn",
            dst_path=CLOUD_OWNERS_LATEST_FOLDER,
            force=True,
            yt_token=YT_TOKEN,
            mr_account="cloud_analytics",
        )

        op_mr_table_path_to_text = vh.op(id="a1691dbb-0d03-4cbf-9acf-5fd2be876f12")
        mr_table_txt = op_mr_table_path_to_text(
            _name="MRTable path -> text",
            yt_token=YT_TOKEN,
            mr_account="cloud_analytics",
            input=mr_table
        )

        op_create_yt_path_arg = vh.op(id="9cb094fb-5b94-11e6-b9c9-0025909427cc")
        yt_path_arg = op_create_yt_path_arg(
            _name="create --yt_path pydriver arg",
            scripts=["s/^/--yt_path /"],
            files=[mr_table_txt]
        )

        op_spark = get_default_spark_op_pg_1secret(PYSPYT)
        op_spark(
            _name="Spark Cloud Creators YT -> PG",
            spyt_py_driver_file=f"{SPYT_JOBS_YT_ROOT}/ods/pg/import/iam/cloud_creator.py",
            py_driver_args=[yt_path_arg]
        )

    # run_config["workflow_guid"] = "262db0ef-e9d3-4330-9260-af137da74444"
    run_config = dict(
        label=WORKFLOW_LABEL,
        workflow_tags=["raw", SOURCE_SYSTEM_NAME]
    )

    reaction_name = WORKFLOW_LABEL.lower().replace(" ", "_")
    reaction_path = f'{ctx.reactor_path_prefix}other/{SOURCE_SYSTEM_NAME}/{reaction_name}'

    return DeployContext(
        run_config=run_config,
        graph=g,
        reaction_path=reaction_path,
        schedule=WORKFLOW_SCHEDULE
    )
