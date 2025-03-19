import logging
from datetime import datetime
from datetime import timedelta

import vh
from yt import wrapper as yt

from cloud.dwh.nirvana.config import DeployContext
from cloud.dwh.nirvana.config import NirvanaDeployContext
from cloud.dwh.nirvana.vh.config.base import BaseDeployConfig as DeployConfig
from cloud.dwh.nirvana.vh.config.base import YT_TOKEN

WORKFLOW_LABEL = "YT cleanup history tables"
WORKFLOW_SCHEDULE = "0 0 * * * ? *"

LEAVE_AT_LEAST_ONE_TABLE = True


def delete_older_than(yt_token, yt_path, timestamp, recursive=True):
    logging.getLogger().setLevel(logging.INFO)

    yt.config["proxy"]["url"] = "hahn"
    yt.config["token"] = yt_token.value

    def loop(folder):
        node_list = yt.list(folder, attributes=["creation_time", "type"], absolute=True)
        table_list = [table for table in node_list if table.attributes["type"] == "table"]
        folder_list = [table for table in node_list if table.attributes["type"] == "map_node"]
        table_list.sort(key=lambda table: datetime.strptime(table.attributes["creation_time"], "%Y-%m-%dT%H:%M:%S.%fZ"))
        table_list_for_removal = []

        for table in table_list:
            creation_time = timestamp.strptime(table.attributes["creation_time"], "%Y-%m-%dT%H:%M:%S.%fZ")
            if creation_time < timestamp:
                table_list_for_removal.append(table)

        if LEAVE_AT_LEAST_ONE_TABLE and len(table_list_for_removal) > 0:
            del table_list_for_removal[-1]

        for table in table_list_for_removal:
            logging.info(f"deleting {table}")
            yt.remove(table)

        if recursive:
            for folder in folder_list:
                loop(folder)

    loop(yt_path)


@vh.lazy(object, current_datetime=vh.mkinput(vh.TextFile), yt_token=vh.Secret)
def cleanup_raw(current_datetime, yt_token):
    timestamp = datetime.now() - timedelta(hours=12)
    delete_older_than(yt_token, "//home/cloud_analytics/dwh/raw/backoffice", timestamp)
    delete_older_than(yt_token, "//home/cloud_analytics/dwh/raw/compute", timestamp)
    delete_older_than(yt_token, "//home/cloud_analytics/dwh/raw/crm", timestamp)


@vh.lazy(object, current_datetime=vh.mkinput(vh.TextFile), yt_token=vh.Secret)
def cleanup_scoring(current_datetime, yt_token):
    timestamp = datetime.now() - timedelta(days=30)
    delete_older_than(yt_token, "//home/cloud_analytics/scoring_v2/AB_control_leads", timestamp, recursive=False)
    delete_older_than(yt_token, "//home/cloud_analytics/scoring_v2/AB_test_leads", timestamp, recursive=False)
    delete_older_than(yt_token, "//home/cloud_analytics/scoring_v2/all_scoring_leads", timestamp, recursive=False)
    delete_older_than(yt_token, "//home/cloud_analytics/scoring_v2/crm_leads", timestamp, recursive=False)


@vh.lazy(object, current_datetime=vh.mkinput(vh.TextFile), yt_token=vh.Secret)
def cleanup_smb(current_datetime, yt_token):
    timestamp = datetime.now() - timedelta(days=30)
    delete_older_than(yt_token, "//home/cloud_analytics/smb/targets_from_mass", timestamp, recursive=False)
    delete_older_than(yt_token, "//home/cloud_analytics/smb/targets_from_mass/train_features", timestamp, recursive=False)
    delete_older_than(yt_token, "//home/cloud_analytics/smb/targets_from_mass/threshold_consumers", timestamp, recursive=False)
    delete_older_than(yt_token, "//home/cloud_analytics/smb/targets_from_mass/predict_features", timestamp, recursive=False)


@vh.lazy(object, current_datetime=vh.mkinput(vh.TextFile), yt_token=vh.Secret)
def cleanup_antifraud(current_datetime, yt_token):
    timestamp = datetime.now() - timedelta(days=30)
    delete_older_than(yt_token, "//home/cloud_analytics/antifraud_suspension/backup_new_format", timestamp, recursive=False)


@vh.lazy(object, current_datetime=vh.mkinput(vh.TextFile), yt_token=vh.Secret)
def cleanup_crm(current_datetime, yt_token):
    timestamp = datetime.now() - timedelta(days=10)
    delete_older_than(yt_token, "//home/cloud_analytics/export/crm/forms_registration", timestamp, recursive=False)
    delete_older_than(yt_token, "//home/cloud_analytics/export/crm/mdb_choose_db_promo", timestamp, recursive=False)
    delete_older_than(yt_token, "//home/cloud_analytics/export/crm/compete-promo", timestamp, recursive=False)


def main(ctx: DeployConfig):
    with vh.Graph() as g:
        yt_token = vh.add_global_option('yt_token', 'secret', default=YT_TOKEN)

        op_get_current_datetime = vh.op(id="87c1dc43-461e-4de3-8f26-c0d1406165b6")
        cur_datetime = op_get_current_datetime(
            date="today",
            format="%Y-%m-%dT%H:%M:%S",
            utc=True
        )

        cleanup_raw(current_datetime=cur_datetime, yt_token=yt_token)
        cleanup_scoring(current_datetime=cur_datetime, yt_token=yt_token)
        cleanup_smb(current_datetime=cur_datetime, yt_token=yt_token)
        cleanup_antifraud(current_datetime=cur_datetime, yt_token=yt_token)
        cleanup_crm(current_datetime=cur_datetime, yt_token=yt_token)

        run_config = NirvanaDeployContext(
            label=WORKFLOW_LABEL,
            workflow_tags=("raw",),
            workflow_guid="38a650b9-100f-4083-b7ed-4f2f8cb48042"
        )

        reaction_name = WORKFLOW_LABEL.lower().replace(" ", "_")
        reaction_path = f'{ctx.reactor_path_prefix}other/{reaction_name}'

        return DeployContext(
            run_config=run_config,
            graph=g,
            reaction_path=reaction_path,
            schedule=WORKFLOW_SCHEDULE
        )
