import pandas as pd
import vh

from cloud.dwh.nirvana.vh.common.chyt import ChytClient
from cloud.dwh.nirvana.vh.common.chyt import ClientConfig
from cloud.dwh.nirvana.vh.common.yt import save_table
from cloud.dwh.nirvana.vh.config.base import YT_TOKEN
from cloud.dwh.nirvana.vh.config.base import BaseDeployConfig as DeployConfig
from cloud.dwh.nirvana.config import DeployContext

WORKFLOW_LABEL = "CDM fct_client_support_request"
WORKFLOW_SCHEDULE = "0 0 */6 * * ? *"

YT_PROXY = "hahn"
YT_PATH = "//home/cloud_analytics/dwh/cdm/fct_client_support_request"


def create_support_first_table(chyt_client):
    support_df1 = chyt_client.execute_query("""
    SELECT
        billing_account_id,
        email,
        puid,
        count(DISTINCT st_key) as  support_tickets_num,
        support_tickets_num / days_since_ba_created  * 30
        as support_tickets_num_per_month
    FROM (
        SELECT
            cloud_id,
            billing_account_id,
            email,
            puid,
            st_key,
            days_since_ba_created
        FROM "//home/cloud/billing/exported-support-tables/tickets_prod" as a
        INNER JOIN (
            SELECT
                DISTINCT
                cloud_id,
                billing_account_id,
                user_settings_email as email,
                puid,
                if (
                toDate(NOW()) - toDate(first_ba_created_datetime) > 18000, -1,
                toDate(NOW()) - toDate(first_ba_created_datetime)
                )
                    as days_since_ba_created
            FROM "//home/cloud_analytics/cubes/acquisition_cube/cube"
            WHERE event == 'ba_created'
            AND billing_account_id != ''
            AND cloud_id != ''
        ) as b
        ON a.cloud_id == b.cloud_id
    )
    GROUP BY billing_account_id, email, puid, days_since_ba_created
    FORMAT TabSeparatedWithNames
    """)
    return support_df1


def create_support_senond_table(chyt_client):
    support_df2 = chyt_client.execute_query("""
    SELECT
        DISTINCT
            current_support_type,
            previous_support_type,
            all_paid_support_types,
            billing_account_id,
            email,
            puid
    FROM (
        SELECT
            MAX(
                if (
                    toDate(end_time) > toDate(NOW()) or isNull(end_time),
                    replaceRegexpOne(fixed_consumption_schema,
                    '[^.]*.([^.]*)..*', '\\1'),
                    null
                )
            ) as current_support_type,

            arrayReverseSort(
                x -> x.2,
                arrayFilter(x -> isNotNull(x.1),
                    groupArray(
                        (
                            if (toDate(end_time) < toDate(NOW()),
                            fixed_consumption_schema, null),
                            toDate(end_time)
                        )
                    )
                )
            ) as history,
            replaceRegexpOne(history[1].1, '[^.]*.([^.]*)..*',
            '\\1') as previous_support_type,
            arrayStringConcat(groupUniqArray(
                replaceRegexpOne(fixed_consumption_schema,
                '[^.]*.([^.]*)..*', '\\1')
            ), ',') as all_paid_support_types,
            billing_account_id,
            email,
            puid
        FROM
        "//home/cloud/billing/exported-billing-tables/support_subscriptions_prod"
        as support_types
        INNER JOIN (
            SELECT
                DISTINCT
                billing_account_id,
                user_settings_email as email,
                puid,
                if (
                toDate(NOW()) - toDate(first_ba_created_datetime) > 18000, -1,
                    toDate(NOW()) - toDate(first_ba_created_datetime))
                    as days_since_ba_created
            FROM "//home/cloud_analytics/cubes/acquisition_cube/cube"
            WHERE event == 'ba_created'
            AND puid != ''
        ) as main_info
        ON support_types.billing_account_id == main_info.billing_account_id
        GROUP BY billing_account_id, email, puid
    )
    FORMAT TabSeparatedWithNames
    """)
    return support_df2


def create_df(yt_token):
    client = ChytClient(config=ClientConfig(
        cluster="hahn",
        clique="*cloud_analytics",
        yt_token=yt_token,
    ))
    support_df1 = create_support_first_table(client)
    support_df2 = create_support_senond_table(client)
    return pd.merge(support_df1,
                    support_df2,
                    on=['billing_account_id',
                        'email', 'puid'],
                    how='outer')


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
        workflow_tags=["cdm", "fct_client_support_request"]
    )
    reaction_name = WORKFLOW_LABEL.lower().replace(" ", "_")
    reaction_path = f'{ctx.reactor_path_prefix}cdm/yt/{reaction_name}'

    return DeployContext(
        run_config=run_config,
        graph=g,
        reaction_path=reaction_path,
        schedule=WORKFLOW_SCHEDULE
    )
