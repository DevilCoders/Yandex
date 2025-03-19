import vh

from cloud.dwh.nirvana.vh.common.chyt import ChytClient
from cloud.dwh.nirvana.vh.common.chyt import ClientConfig
from cloud.dwh.nirvana.vh.common.yt import save_table
from cloud.dwh.nirvana.vh.config.base import YT_TOKEN
from cloud.dwh.nirvana.vh.config.base import BaseDeployConfig as DeployConfig
from cloud.dwh.nirvana.config import DeployContext

WORKFLOW_LABEL = "CDM fct_client_ba_info"
WORKFLOW_SCHEDULE = "0 0 */6 * * ? *"

YT_PROXY = "hahn"
YT_PATH = "//home/cloud_analytics/dwh/cdm/fct_client_ba_info"


def create_df(yt_token):
    client = ChytClient(config=ClientConfig(
        cluster=YT_PROXY,
        clique="*cloud_analytics",
        yt_token=yt_token,
    ))
    df = client.execute_query("""
        SELECT
        DISTINCT
            billing_account_id,
            user_settings_email as email,
            puid,
            cloud_id,
            segment,
            event_time as console_regstration_date,
            if(first_ba_created_datetime = '0000-00-00 00:00:00', '',
               first_ba_created_datetime) as ba_created,
            if (toDate(NOW()) - toDate(first_ba_created_datetime) > 18000, -1,
                toDate(NOW()) - toDate(first_ba_created_datetime))
                as days_since_ba_created,

            if(isNull(start_grant_datetime), null, start_grant_datetime)
            as start_grant_datetime,

            if(first_first_trial_consumption_datetime != '0000-00-00 00:00:00',
            first_first_trial_consumption_datetime, '')
            as start_trial_consumption_datetime,

            if(isNull(end_grant_datetime), null, end_grant_datetime)
            as end_grant_datetime,

            if(first_first_paid_consumption_datetime != '0000-00-00 00:00:00',
            first_first_paid_consumption_datetime, '')
            as start_paid_consumption_datetime,

            ba_usage_status,
            ba_state,
            block_reason,
            ba_person_type,
            ba_type,
            cloud_status,
            if (sales_name == '' or sales_name == 'unmanaged', 0, 1) as is_managed
    FROM "//home/cloud_analytics/cubes/acquisition_cube/cube" as cube_info
    LEFT JOIN (
        SELECT
        max(toDateTime(end_time)) as end_grant_datetime,
        min(toDateTime(start_time)) as start_grant_datetime,
        billing_account_id
        FROM "//home/cloud/billing/exported-billing-tables/monetary_grants_prod"
        WHERE source == 'default'
        GROUP BY billing_account_id
    ) trial_info
    ON cube_info.billing_account_id == trial_info.billing_account_id
    WHERE (
            (event == 'cloud_created' and  billing_account_id in
             (SELECT DISTINCT billing_account_id
              FROM "//home/cloud_analytics/cubes/acquisition_cube/cube"
              WHERE event == 'cloud_created')
             )
          OR
            (event == 'ba_created' and  billing_account_id not in
             (SELECT DISTINCT billing_account_id
              FROM "//home/cloud_analytics/cubes/acquisition_cube/cube"
              WHERE event == 'cloud_created')
            )
        )
    FORMAT TabSeparatedWithNames
    """)
    return df


@vh.lazy.hardware_params(vh.HardwareParams(max_ram=4000))
@vh.lazy(object, current_datetime=vh.mkinput(vh.TextFile), yt_token=vh.Secret)
def create_and_save_df(current_datetime, yt_token):
    df = create_df(yt_token.value)
    save_table(YT_PROXY, yt_token.value, YT_PATH, df)


def main(ctx: DeployConfig):
    with vh.Graph() as g:
        yt_token = vh.add_global_option('yt_token', 'secret', default=YT_TOKEN)
        op_get_current_datetime = vh.op(id="87c1dc43-461e-4de3-8f26-c0d1406165b6")
        cur_datetime = op_get_current_datetime(
            date="today",
            format="%Y-%m-%dT%H:%M:%S",
            utc=True
        )
        create_and_save_df(current_datetime=cur_datetime, yt_token=yt_token)

    # run_config["workflow_guid"] = "bb959512-382f-489e-adf9-607c154cf15d"
    run_config = dict(
        label=WORKFLOW_LABEL,
        workflow_tags=["cdm", "fct_client_ba_info"]
    )
    reaction_name = WORKFLOW_LABEL.lower().replace(" ", "_")
    reaction_path = f'{ctx.reactor_path_prefix}cdm/yt/{reaction_name}'

    return DeployContext(
        run_config=run_config,
        graph=g,
        reaction_path=reaction_path,
        schedule=WORKFLOW_SCHEDULE
    )
