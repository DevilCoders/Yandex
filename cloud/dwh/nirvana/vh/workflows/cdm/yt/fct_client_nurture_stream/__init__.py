import vh

from cloud.dwh.nirvana.vh.common.chyt import ChytClient
from cloud.dwh.nirvana.vh.common.chyt import ClientConfig
from cloud.dwh.nirvana.vh.common.yt import save_table
from cloud.dwh.nirvana.vh.config.base import YT_TOKEN
from cloud.dwh.nirvana.vh.config.base import BaseDeployConfig as DeployConfig
from cloud.dwh.nirvana.config import DeployContext

WORKFLOW_LABEL = "CDM fct_client_nurture_stream"
WORKFLOW_SCHEDULE = "0 0 */6 * * ? *"

YT_PROXY = "hahn"
YT_PATH = "//home/cloud_analytics/dwh/cdm/fct_client_nurture_stream"


def create_df(yt_token):
    client = ChytClient(config=ClientConfig(
        cluster=YT_PROXY,
        clique="*cloud_analytics",
        yt_token=yt_token,
    ))
    df = client.execute_query("""
        SELECT
        *
        FROM (
            SELECT
                DISTINCT
                    billing_account_id,
                    puid,
                    email,
                    'Test' as Group,
                    program_name,
                    toString(groupUniqArray(mailing_name)) as sended_letters,
                    min(toString(delivery_time)) as add_to_programm_date,
                    min(toString(delivery_time)) as first_letter_date,
                    max(toString(delivery_time)) as last_letter_date,
                    sum(is_open) as num_of_opened_letters,
                    sum(is_click) as num_of_clicked_letters,
                    avg(is_open) * 100 as pct_of_opened_letters,
                    avg(is_click) * 100 as pct_of_clicked_letters
            FROM (
                SELECT
                    billing_account_id,
                    puid,
                    email,
                    program_name,
                    stream_name,
                    mailing_name,
                    is_open,
                    is_click,
                    delivery_time
                FROM (
                    SELECT
                        DISTINCT
                        billing_account_id,
                        puid,
                        multiIf(email LIKE '%@yandex.%' OR email LIKE '%@ya.%',
                                CONCAT(lower(replaceAll(splitByString('@',assumeNotNull(email))[1], '.', '-')), '@yandex.ru'),
                                lower(email)
                                ) as email,
                        program_name,
                        stream_name,
                        mailing_name
                    FROM "//home/cloud_analytics/cubes/emailing_events/emailing_events"
                WHERE event == 'add_to_nurture_stream'
                ) as sream_info
                INNER JOIN (
                    SELECT
                        multiIf(email LIKE '%@yandex.%' OR email LIKE '%@ya.%',
                                CONCAT(lower(replaceAll(splitByString('@',assumeNotNull(email))[1], '.', '-')), '@yandex.ru'),
                                lower(email)
                                ) as email,
                        tags,
                        max(if(event == 'px', 1, 0)) as is_open,
                        max(if(event == 'click', 1, 0)) as is_click,
                        min(if(toDate(unixtime / 1000) >= '2020-01-01', toDate(unixtime / 1000), null)) as delivery_time
                    FROM "//home/cloud_analytics/import/emails/emails_delivery_clicks"
                    GROUP BY tags, email
                ) as letter_info
                ON letter_info.email == sream_info.email and letter_info.tags == sream_info.mailing_name
                WHERE stream_name == 'Test'
            )
            GROUP BY billing_account_id, puid, email, program_name

            UNION ALL

            SELECT
                DISTINCT
                    billing_account_id,
                    puid,
                    multiIf(email LIKE '%@yandex.%' OR email LIKE '%@ya.%',
                            CONCAT(lower(replaceAll(splitByString('@',assumeNotNull(email))[1], '.', '-')), '@yandex.ru'),
                            lower(email)
                            ) as email,
                    'Control' as Group,
                    program_name,
                    toString([]) as sended_letters,
                    min(toString(toDate(event_time))) as add_to_programm_date,
                    null as first_letter_date,
                    null as last_letter_date,
                    null as num_of_opened_letters,
                    null as num_of_clicked_letters,
                    null as pct_of_opened_letters,
                    null as pct_of_clicked_letters
            FROM "//home/cloud_analytics/cubes/emailing_events/emailing_events"
            WHERE event == 'add_to_nurture_stream'
            AND stream_name == 'Control'
            GROUP BY billing_account_id, puid, email, program_name
        )
        ORDER BY email, add_to_programm_date
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
        workflow_tags=["cdm", "fct_client_nurture_stream"]
    )
    reaction_name = WORKFLOW_LABEL.lower().replace(" ", "_")
    reaction_path = f'{ctx.reactor_path_prefix}cdm/yt/{reaction_name}'

    return DeployContext(
        run_config=run_config,
        graph=g,
        reaction_path=reaction_path,
        schedule=WORKFLOW_SCHEDULE
    )
