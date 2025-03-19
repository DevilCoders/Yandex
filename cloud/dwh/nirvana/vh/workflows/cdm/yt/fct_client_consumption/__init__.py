import vh
import yt.wrapper as yt

from cloud.dwh.nirvana.config import DeployContext
from cloud.dwh.nirvana.config import NirvanaDeployContext
from cloud.dwh.nirvana.vh.common.chyt import ChytClient
from cloud.dwh.nirvana.vh.common.chyt import ClientConfig
from cloud.dwh.nirvana.vh.common.yt import save_table
from cloud.dwh.nirvana.vh.config.base import BaseDeployConfig as DeployConfig
from cloud.dwh.nirvana.vh.config.base import YT_TOKEN

WORKFLOW_LABEL = "CDM fct_client_consumption"
WORKFLOW_SCHEDULE = "0 0 */6 * * ? *"

YT_PROXY = "hahn"
YT_PATH_TRIAL = "//home/cloud_analytics/dwh/cdm/fct_client_consumption_trial"
YT_PATH_PAID = "//home/cloud_analytics/dwh/cdm/fct_client_consumption_paid"
YT_PATH_TRIAL_LAST_DAYS = "//home/cloud_analytics/dwh/cdm/fct_client_consumption_last_days_trial"
YT_PATH_PAID_LAST_DAYS = "//home/cloud_analytics/dwh/cdm/fct_client_consumption_last_days_paid"


def sanitize_name(name: str):
    return str(name).replace(".", "_").replace("-", "_")


def consumption_request_part_for_service(type_cons: str,
                                         service: str,
                                         service_name: str):
    assert service in ['service_name', 'subservice_name']

    part_req = f"""
    SUM(if({service} == '{service_name}',
    {type_cons}_consumption, 0)) as
    full_{type_cons}_consumption_{service}_{sanitize_name(service_name)},

    full_{type_cons}_consumption_{service}_{sanitize_name(service_name)} /
    {type_cons}_duartion_in_days as
    {type_cons}_per_day_consumption_{service}_{sanitize_name(service_name)},

    {type_cons}_per_day_consumption_{service}_{sanitize_name(service_name)} * 7
    as {type_cons}_per_week_consumption_{service}_{sanitize_name(service_name)},

    {type_cons}_per_day_consumption_{service}_{sanitize_name(service_name)} * 30
    as {type_cons}_per_month_consumption_{service}_{sanitize_name(service_name)},
    """
    return part_req


def consumption_table_request_maker(type_cons, services, subservices):
    full_adding_req: str = ""
    for service in services:
        full_adding_req += consumption_request_part_for_service(type_cons,
                                                                'service_name',
                                                                service)
    for subservice in subservices:
        full_adding_req += \
            consumption_request_part_for_service(type_cons,
                                                 'subservice_name',
                                                 subservice)

    request = f"""
    SELECT
        DISTINCT
            billing_account_id,
            periods.email as email,
            periods.puid as puid,
            {full_adding_req}

    ifNull(SUM({type_cons}_consumption), 0) as full_{type_cons}_consumption,
            full_{type_cons}_consumption / {type_cons}_duartion_in_days
            as {type_cons}_per_day_consumption,

            {type_cons}_per_day_consumption * 7
            as {type_cons}_per_week_consumption,

            {type_cons}_per_day_consumption * 30
            as {type_cons}_per_month_consumption

    FROM "//home/cloud_analytics/cubes/acquisition_cube/cube" as main_cons
    INNER JOIN (
        SELECT
            DISTINCT
                billing_account_id,
                user_settings_email as email,
                puid,
                ifNull(trial_duartion_in_days, 1) as trial_duartion_in_days,

                toDate(NOW()) - toDate(first_first_paid_consumption_datetime) + 1
                as raw_days_since_paid_starts,

                if(
                    raw_days_since_paid_starts > 18000 or
                    raw_days_since_paid_starts == 0,
                1, raw_days_since_paid_starts + 1
                ) as real_duartion_in_days
        FROM "//home/cloud_analytics/cubes/acquisition_cube/cube" as cube_info
        LEFT JOIN (
            SELECT
            SUM(toDate(end_time) - toDate(start_time) + 1) as trial_duartion_in_days,
            billing_account_id
            FROM "//home/cloud/billing/exported-billing-tables/monetary_grants_prod"
            GROUP BY billing_account_id
        ) as trial_info
        ON cube_info.billing_account_id == trial_info.billing_account_id
        WHERE isNotNull(billing_account_id)
        AND event == 'ba_created'
    ) as periods
    ON main_cons.billing_account_id == periods.billing_account_id
    GROUP BY billing_account_id, periods.email, periods.puid,
    trial_duartion_in_days, real_duartion_in_days
    FORMAT TabSeparatedWithNames
    """
    return request


def consumption_request_part_for_service_last_days(type_cons: str,
                                                   service: str,
                                                   service_name: str):
    assert service in ['service_name', 'subservice_name']

    part_req = f"""
    SUM(if({service} == '{service_name}' and toDate(event_time) > addDays(toDate(NOW()), -15),
    {type_cons}_consumption, 0)) as {type_cons}_per_last_15_days_{service}_{sanitize_name(service_name)},

    SUM(if({service} == '{service_name}' and toDate(event_time) > addDays(toDate(NOW()), -30),
    {type_cons}_consumption, 0)) as {type_cons}_per_last_30_days_{service}_{sanitize_name(service_name)},

    SUM(if({service} == '{service_name}' and toDate(event_time) > addDays(toDate(NOW()), -60),
    {type_cons}_consumption, 0)) as {type_cons}_per_last_60_days_{service}_{sanitize_name(service_name)},
    """
    return part_req


def consumption_table_request_maker_last_days(type_cons, services, subservices):
    full_adding_req = ""
    for service in services:
        full_adding_req += \
            consumption_request_part_for_service_last_days(type_cons,
                                                           'service_name',
                                                           service)
    for subservice in subservices:
        full_adding_req += \
            consumption_request_part_for_service_last_days(type_cons,
                                                           'subservice_name',
                                                           subservice)
    request = f"""
        SELECT
            DISTINCT
                billing_account_id,
                periods.email as email,
                periods.puid as puid,
                {full_adding_req}

                SUM(if(toDate(event_time) >= addDays(toDate(NOW()), -15),
                    {type_cons}_consumption, 0)) as full_{type_cons}_per_last_15_days,

                SUM(if(toDate(event_time) >= addDays(toDate(NOW()), -30),
                        {type_cons}_consumption, 0)) as full_{type_cons}_per_last_30_days,

                SUM(if(toDate(event_time) >= addDays(toDate(NOW()), -60),
                        {type_cons}_consumption, 0)) as full_{type_cons}_per_last_60_days
        FROM "//home/cloud_analytics/cubes/acquisition_cube/cube" as main_cons
        INNER JOIN (
        SELECT
            DISTINCT
                billing_account_id,
                user_settings_email as email,
                puid,
                ifNull(trial_duartion_in_days, 1) as trial_duartion_in_days,

                toDate(NOW()) - toDate(first_first_paid_consumption_datetime) + 1
                as raw_days_since_paid_starts,

                if(
                    raw_days_since_paid_starts > 18000 or
                    raw_days_since_paid_starts == 0,
                1, raw_days_since_paid_starts + 1
                ) as real_duartion_in_days
        FROM "//home/cloud_analytics/cubes/acquisition_cube/cube" as cube_info
        LEFT JOIN (
            SELECT
            SUM(toDate(end_time) - toDate(start_time) + 1) as trial_duartion_in_days,
            billing_account_id
            FROM "//home/cloud/billing/exported-billing-tables/monetary_grants_prod"
            GROUP BY billing_account_id
        ) as trial_info
        ON cube_info.billing_account_id == trial_info.billing_account_id
        WHERE isNotNull(billing_account_id)
        AND event == 'ba_created'
        ) as periods
        ON main_cons.billing_account_id == periods.billing_account_id
        GROUP BY billing_account_id, periods.email, periods.puid,
        trial_duartion_in_days, real_duartion_in_days
        FORMAT TabSeparatedWithNames
    """
    return request


def create_df(yt_token: str, type_cons: str, request_maker):
    client = ChytClient(config=ClientConfig(
        cluster="hahn",
        clique="*cloud_analytics",
        yt_token=yt_token,
    ))

    services = client.execute_query("""
        SELECT
            DISTINCT
                service_name
        FROM "//home/cloud_analytics/cubes/acquisition_cube/cube"
        FORMAT TabSeparatedWithNames
    """)

    subservices = client.execute_query("""
        SELECT
            DISTINCT
                subservice_name
        FROM "//home/cloud_analytics/cubes/acquisition_cube/cube"
        FORMAT TabSeparatedWithNames
    """)

    services = list(services['service_name'])
    subservices = list(subservices['subservice_name'])
    query = request_maker(type_cons, services, subservices)
    cons_table = client.execute_query(query)
    return cons_table


def create_and_save_df(yt_token: str, type_cons: str, request_maker, yt_path: str):
    df = create_df(yt_token, type_cons, request_maker)
    save_table(YT_PROXY, yt_token, yt_path, df)


@vh.lazy.hardware_params(vh.HardwareParams(max_ram=20000))
@vh.lazy(object, current_datetime=vh.mkinput(vh.TextFile), yt_token=vh.Secret)
def create_and_save_df_paid(current_datetime, yt_token):
    create_and_save_df(yt_token.value, "real", consumption_table_request_maker, YT_PATH_PAID)
    return yt.TablePath(YT_PATH_PAID)


@vh.lazy.hardware_params(vh.HardwareParams(max_ram=20000))
@vh.lazy(object, current_datetime=vh.mkinput(vh.TextFile), yt_token=vh.Secret)
def create_and_save_df_trial_last_days(current_datetime, yt_token):
    create_and_save_df(yt_token.value, "trial", consumption_table_request_maker_last_days, YT_PATH_TRIAL_LAST_DAYS)
    return yt.TablePath(YT_PATH_TRIAL_LAST_DAYS)


@vh.lazy.hardware_params(vh.HardwareParams(max_ram=20000))
@vh.lazy(object, current_datetime=vh.mkinput(vh.TextFile), yt_token=vh.Secret)
def create_and_save_df_paid_last_days(current_datetime, yt_token):
    create_and_save_df(yt_token.value, "real", consumption_table_request_maker_last_days, YT_PATH_PAID_LAST_DAYS)
    return yt.TablePath(YT_PATH_PAID_LAST_DAYS)


@vh.lazy.hardware_params(vh.HardwareParams(max_ram=20000))
@vh.lazy(object, current_datetime=vh.mkinput(vh.TextFile), yt_token=vh.Secret)
def create_and_save_df_trial(current_datetime, yt_token):
    create_and_save_df(yt_token.value, "trial", consumption_table_request_maker, YT_PATH_TRIAL)
    return yt.TablePath(YT_PATH_TRIAL)


def main(ctx: DeployConfig):
    with vh.Graph() as g:
        yt_token = vh.add_global_option('yt_token', 'secret', default=YT_TOKEN)
        op_get_current_datetime = vh.op(id="87c1dc43-461e-4de3-8f26-c0d1406165b6")
        cur_datetime = op_get_current_datetime(
            date="today",
            format="%Y-%m-%dT%H:%M:%S",
            utc=True
        )
        create_and_save_df_paid(current_datetime=cur_datetime, yt_token=yt_token)
        create_and_save_df_trial(current_datetime=cur_datetime, yt_token=yt_token)
        create_and_save_df_trial_last_days(current_datetime=cur_datetime, yt_token=yt_token)
        create_and_save_df_paid_last_days(current_datetime=cur_datetime, yt_token=yt_token)

    run_config = NirvanaDeployContext(
        workflow_guid='f61dd8e6-8efc-48ad-a9c9-504dd9351389',
        label=WORKFLOW_LABEL,
        workflow_tags=('cdm', 'fct_client_consumption'),
    )

    reaction_name = WORKFLOW_LABEL.lower().replace(" ", "_")
    reaction_path = f'{ctx.reactor_path_prefix}cdm/yt/{reaction_name}'

    return DeployContext(
        run_config=run_config,
        graph=g,
        reaction_path=reaction_path,
        schedule=WORKFLOW_SCHEDULE
    )
