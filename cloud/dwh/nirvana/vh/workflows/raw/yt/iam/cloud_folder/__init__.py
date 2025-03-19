import vh

from cloud.dwh.nirvana.vh.config.base import YT_TOKEN
from cloud.dwh.nirvana.vh.config.base import BaseDeployConfig as DeployConfig
from cloud.dwh.nirvana.config import DeployContext

WORKFLOW_LABEL = "REST IAM cloud_folder IAM to YT"
SOURCE_SYSTEM_NAME = "iam"

WORKFLOW_SCHEDULE = "0 0 4 * * ? *"

CLOUD_FOLDERS_FOLDER = "//home/cloud_analytics/import/iam/cloud_folders/1h"
CLOUD_FOLDERS_LATEST_FOLDER = CLOUD_FOLDERS_FOLDER + "/latest"


def main(ctx: DeployConfig):
    with vh.Graph() as g:
        op_cloud_folders_to_yt = vh.op(id="14fd1aa9-4bff-455b-8fe0-e31e2f822dd3")
        mr_table = op_cloud_folders_to_yt(
            _name="IAM Cloud Folders -> YT (sandbox task)",
            sandbox_oauth_token="robot-clanalytics-sandbox",
            owner="CLOUD_ANALYTICS",
            # Destination_YT_cluster="hahn",
            dst_cluster="hahn",
            # Destination_YT_prefix_for_CloudFolders_tables=CLOUD_FOLDERS_FOLDER,
            cloud_folders_dst_yt_prefix=CLOUD_FOLDERS_FOLDER,
            # Cloud_creators_table_to_get_cloud_ids_from="//home/cloud_analytics/import/iam/cloud_owners/1h/latest",
            cloud_creators_table="//home/cloud_analytics/import/iam/cloud_owners/1h/latest",
            iam_api_endpoint="https://identity.private-api.cloud.yandex.net:14336/v1",
            juggler_host="cloud-analytics-scheduled-jobs",
            yt_token_name="robot-clanalytics-yt-yt-token",
            threads_count=5
        )

        op_mr_copy_table = vh.op(id="23762895-cf87-11e6-9372-6480993f8e34")
        op_mr_copy_table(
            _name="MRTable update latest",
            src=mr_table,
            dst_cluster="hahn",
            dst_path=CLOUD_FOLDERS_LATEST_FOLDER,
            force=True,
            yt_token=YT_TOKEN,
            mr_account="cloud_analytics",
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
