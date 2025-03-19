import vh
import yt.wrapper as yt

from cloud.dwh.nirvana.vh.common.chyt import ChytClient
from cloud.dwh.nirvana.vh.common.chyt import ClientConfig
from cloud.dwh.nirvana.vh.common.yt import save_table
from cloud.dwh.nirvana.vh.config.base import YT_TOKEN
from cloud.dwh.nirvana.vh.config.base import BaseDeployConfig as DeployConfig
from cloud.dwh.nirvana.config import DeployContext

WORKFLOW_LABEL = "CDM fct_client_cloud_webpage_visit"
WORKFLOW_SCHEDULE = "0 0 */6 * * ? *"

YT_PROXY = "hahn"
YT_PATH = "//home/cloud_analytics/dwh/cdm/fct_client_cloud_website_page_visit"


def by_events_get_services_for_interest_pages(events_array, page_name,
                                              interest_dict, main_page_types):
    answer = []
    for event in events_array:
        if page_name in event:
            flag = 0
            for service, value_array in interest_dict.items():
                for value in value_array:
                    if value in event:
                        flag = 1
                        answer.append(value)
                        break
                if flag:
                    break
    return str(list(set(answer)))


def by_events_get_interest_pages_for_services(events_array, service,
                                              interest_dict, main_page_types):
    answer = []
    for event in events_array:
        for value in interest_dict[service]:
            if value in event:
                for page in main_page_types:
                    if page in event:
                        answer.append(page)
    return str(list(set(answer)))


def create_df(yt_token):
    client = ChytClient(config=ClientConfig(
        cluster="hahn",
        clique="*cloud_analytics",
        yt_token=yt_token,
    ))
    interest_dict = {'mssql': ['mssql',
                               'ms sql',
                               'f2emdc91af8p8cpm4tkr',
                               'f2e8cuqa26h8la6c33sq',
                               'f2eovjk4uopcfrsm2e1g'],
                     'postgresql': ['postgresql'],
                     'mysql': ['mysql'],
                     'clickhouse': ['clickhouse'],
                     'mongodb': ['mongodb'],
                     'data-proc': ['data-proc', 'data proc'],
                     'yandex databases': ['ydb', 'yandex database'],
                     'redis': ['redis'],
                     'kafka': ['kafka'],
                     'elastic': ['elastic'],
                     'kubernetes': ['kubernetes'],
                     'container-registry': ['container-registry'],
                     'functions': ['functions'],
                     'message-queue': ['message-queue', 'ymq'],
                     'api-gateway': ['api-gateway'],
                     'iot-core': ['iot-core'],
                     'datasphere': ['datasphere', 'data-sphere'],
                     'object-storage': ['object-storage'],
                     'vpc': ['vpc'],
                     'load-balancer': ['load-balancer'],
                     'kms': ['kms'],
                     'data-transfer': ['data-transfer'],
                     'datalens': ['datalens'],
                     'monitoring': ['monitoring'],
                     'speechkit': ['speechkit'],
                     'translate': ['translate'],
                     'vision': ['vision']
                     }
    main_page_types = ['docs', 'prices',
                       'create-cluster', 'console', 'services']
    main_page_types_as_req_part = str([f'.{x}.' for x in main_page_types])
    interest_pages_as_req = str([item for sublist in interest_dict.values()
                                 for item in sublist])
    sql = f"""
    --sql
        SELECT
            DISTINCT
            billing_account_id,
            puid,
            email,
            groupUniqArray(event) as event_array,
            groupUniqArray(if (event like '%/cases/%', event, null)
            ) as sucess_stories_pages
        FROM (
            SELECT
                billing_account_id,
                puid,
                email,
                splitByChar('?', assumeNotNull(event))[1] as event,
                toDate(replaceRegexpOne(timestamp, '[.].*', '')) as time
            FROM "//home/cloud_analytics/import/console_logs/events" as a
            INNER JOIN (
                SELECT
                    puid,
                    billing_account_id,
                    user_settings_email as email
                FROM "//home/cloud_analytics/cubes/acquisition_cube/cube"
                WHERE event == 'ba_created'
                AND billing_account_id != ''
                AND puid != ''
            ) as b
            ON a.puid == b.puid
            WHERE response >= '200'
            AND response < '300'
            AND (multiMatchAny(event, {main_page_types_as_req_part})
            AND multiMatchAny(event, {interest_pages_as_req}))
            OR (event like '%/cases/%')
        )
        WHERE toDate(time) > addDays(toDate(NOW()), -30)
        GROUP BY billing_account_id, puid, email
        FORMAT TabSeparatedWithNames
    --endsql
    """

    console_df = client.execute_query(sql)

    for page_type in main_page_types:
        column_name = (page_type + "_visits").replace("-", "_").replace(" ", "_")
        console_df[column_name] = console_df['event_array'].apply(
            lambda x: by_events_get_services_for_interest_pages(x, page_type, interest_dict, main_page_types)
        )

    for service in interest_dict:
        column_name = (service + "_associated_pages_visits").replace("-", "_").replace(" ", "_")
        console_df[column_name] = console_df['event_array'].apply(
            lambda x: by_events_get_interest_pages_for_services(x, service, interest_dict, main_page_types)
        )
    console_df['sucess_stories_pages'] = console_df['sucess_stories_pages'].astype(str)

    return console_df


@vh.lazy.hardware_params(vh.HardwareParams(max_ram=2000))
@vh.lazy(object, current_datetime=vh.mkinput(vh.TextFile), yt_token=vh.Secret)
def create_and_save_df(current_datetime, yt_token):
    df = create_df(yt_token.value)
    save_table(YT_PROXY, yt_token.value, YT_PATH, df.drop(columns=['event_array']))
    return yt.TablePath(YT_PATH)


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
        workflow_guid='44759081-7970-4e32-8eff-a3cbcd33aa22',
        workflow_tags=["cdm", "fct_client_cloud_webpage_visit"],
        yt_token=YT_TOKEN
    )

    reaction_name = WORKFLOW_LABEL.lower().replace(" ", "_")
    reaction_path = f'{ctx.reactor_path_prefix}cdm/yt/{reaction_name}'

    return DeployContext(
        run_config=run_config,
        graph=g,
        reaction_path=reaction_path,
        schedule=WORKFLOW_SCHEDULE
    )
