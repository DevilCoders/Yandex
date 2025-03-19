import json
import math
import logging.config
from textwrap import dedent
import click
from clan_tools import utils
# from clan_tools.data_adapters.YQLAdapter import YQLAdapter
from clan_tools.utils.conf import read_conf
from clan_tools.utils.timing import timing

import pandas as pd
from dateutil.parser import parse
from datetime import datetime, timezone
import yt.wrapper as yt

from clan_tools.data_adapters.ClickHouseYTAdapter import ClickHouseYTAdapter




config = read_conf('config/logger.yml')
logging.config.dictConfig(config)
logger = logging.getLogger(__name__)

@timing
@click.command()
@click.option('--write', is_flag=True)
def main(write):
    yt.config["proxy"]["url"] = "hahn"
    ch_adapter = ClickHouseYTAdapter()

    result_table_path = "//home/cloud_analytics/lunin-dv/dashboard_tables"
    result_table_name = "var_info"

    logger.info('Start getting info from CRM')

    # res_table = ['billing_created_time', 
    # mail_billing', 'mail_tech', 'mail_testing', 'mail_feature', 'mail_promo', 'mail_info','mail_event'] +
    #
    # ['date_entered', 'date_qualified', 'account_name',
    # 'first_name', 'last_name', 'title', 'website', 'phone_mobile', 'email',
    # 'description', 'profile', 'num_of_days_from_entered_to_qualified',
    # 'days_to_now', 'weeks_to_now', 'months_to_now', 'architect', 'bus_dev',
    # 'partner_manager', 'sales']


    res_table = pd.merge(get_mail_subscription(ch_adapter), 
                        get_data_from_crm(ch_adapter), 
                        on='billing_account_id', how='inner')


    # res_table = res_table + 
    # ['puid', 'backoffice_email', 'backoffice_phone', 'backoffice_name']
    res_table = pd.merge(res_table, 
                        get_puid_bo_contacts(ch_adapter), 
                        on='billing_account_id', how='left')


    # res_table = res_table + 
    # ['one_month_sub_accounts', 'two_month_sub_accounts', 'three_month_sub_accounts', 'four_month_sub_accounts',
    # 'five_month_sub_accounts', 'six_month_sub_accounts', 'six_plus_month_sub_accounts', 'all_sub_accounts_number']

    invited_subaccounts = get_invited_subaccounts(ch_adapter)

    res_table = pd.merge(res_table, invited_subaccounts, on='billing_account_id', how='left')
    columns = invited_subaccounts.columns[1:]

    res_table[columns] = res_table[columns].fillna(0)
    res_table[columns] = res_table[columns].astype(int)

    # Потребление
    logger.info('Start getting consumption from cube')

    # Подготавливаем интервалы и названия колонок для статитсики
    intervals = ["addQuarters(toStartOfQuarter(NOW()), 1)",
                "toStartOfQuarter(NOW())",
                "addQuarters(toStartOfQuarter(NOW()), -1)",
                "addQuarters(toStartOfQuarter(NOW()), -2)",
                "addQuarters(toStartOfQuarter(NOW()), -3)",
                "addQuarters(toStartOfQuarter(NOW()), -4)"]

            
    # подгатавливаем "суффиксы" для названия колонок
    col_names =  get_last_quarters()


    # res_table = res_table +
    # ['billing_consumption_2020_Q4', 'billing_consumption_2020_Q3', 'billing_consumption_2020_Q2',
    # 'billing_consumption_2020_Q1', 'billing_consumption_2019_Q4']

    ba_consumption = get_ba_consumption(ch_adapter, col_names, intervals)

    res_table = pd.merge(res_table, ba_consumption, on='billing_account_id', how='left')
    res_table[ba_consumption.columns] = res_table[ba_consumption.columns].fillna(0)


    # Добавляем потребление сабаккаунтов

    # res_table = res_table + 
    # ['cons_subaccount_2020_Q4', 'subaccount_num_2020_Q4', 'avg_cons_by_one_subaccount_2020_Q4',
    # 'cons_subaccount_2020_Q3', 'subaccount_num_2020_Q3', 'avg_cons_by_one_subaccount_2020_Q3',
    # 'cons_subaccount_2020_Q2', 'subaccount_num_2020_Q2', 'avg_cons_by_one_subaccount_2020_Q2',
    # 'cons_subaccount_2020_Q1', 'subaccount_num_2020_Q1', 'avg_cons_by_one_subaccount_2020_Q1',
    # 'cons_subaccount_2019_Q4', 'subaccount_num_2019_Q4', 'avg_cons_by_one_subaccount_2019_Q4',

    sub_consumption = get_sub_consumption(ch_adapter, col_names, intervals)

    res_table = pd.merge(res_table, 
                        sub_consumption,
                        on='billing_account_id', how='left')

    res_table[sub_consumption.columns] = res_table[sub_consumption.columns].fillna(0)

    # если по одному биллингу несколько записей - оставляем только одну
    logger.info('Start dropping duplicates')
    res_table = res_table.sort_values(by='billing_account_id')
    res_table.reset_index(drop=True, inplace=True)

    res_table = res_table.drop_duplicates(subset=['billing_account_id'])
    res_table.reset_index(drop=True, inplace=True)
    logger.info('Start saving table')

    save_table(result_table_name, result_table_path, res_table)

    with open('output.json', 'w') as f:
            json.dump({"table_path" : result_table_path + "/" + result_table_name}, f)


def find_tables_in_hahn_folder(path):
    '''возвращает все таблицы в папке'''
    tables = []
    for table in yt.search(path, node_type=["table"],
                           attributes=["account"]):
        tables.append(table)
    return tables

def make_num_of_days_from_entered_to_qualified(row):
    # Смотрим сколько дней прошло между датой entered и qualified

    try:
        days = (parse(row['date_qualified']) - parse(row['date_entered'])).days
        return str(days)
    except Exception:
        return ''
    
def make_days_to_now(x):
    # Смотрим сколько дней прошло между датой qualified и сегодня
    try:
        days = (parse(str(datetime.date(datetime.now()))) - parse(x)).days
        return str(days)
    except Exception:
        return 'no_data'
    
def make_weeks(x):
    # Смотрим сколько недель прошло между датой qualified и сегодня
    try:
        return str(round(int(x) / 7, 2))
    except Exception:
        return 'no_data'
    
def make_months(x):
    # Смотрим сколько месяцев прошло между датой qualified и сегодня
    try:
        return str(round(int(x) / 30, 2))
    except Exception:
        return 'no_data'
    
def iterate_by_two(intervals):
    for i in range(len(intervals) - 1):
        yield intervals[i + 1], intervals[i]

def get_last_quarters():
    # получаем суффиксы с годом и кварталом для колонок с измерениями потребления
    quarter = math.ceil(datetime.now().month/3)
    year = datetime.now().year

    if quarter==4:
        return[f'{year}_Q4', f'{year}_Q3', f'{year}_Q2', f'{year}_Q1', f'{year-1}_Q4']
    if quarter==3:
         return[f'{year}_Q3', f'{year}_Q2', f'{year}_Q1', f'{year-1}_Q4', f'{year-1}_Q3']
    if quarter==2:
         return[f'{year}_Q2', f'{year}_Q1', f'{year-1}_Q4', f'{year-1}_Q3', f'{year-1}_Q2']
    else:
        return[f'{year}_Q1', f'{year-1}_Q4', f'{year-1}_Q3', f'{year-1}_Q2', f'{year-1}_Q1']

def time_to_unix(time_str):
    dt = parse(time_str)
    timestamp = dt.replace(tzinfo=timezone.utc).timestamp()
    return int(timestamp)

def get_data_from_crm(ch_adapter):
    '''
     Собирает из CRM и не только информацию по заведенным VAR-аккаунтам   
    '''

    logger.info('Start get_data_from_crm')

    leads_table = find_tables_in_hahn_folder("//home/cloud_analytics/dwh/raw/crm/leads")[-1]
    billing_leads_keys = find_tables_in_hahn_folder("//home/cloud_analytics/dwh/raw/crm/leads_billing_accounts")[-1]
    billing_table = find_tables_in_hahn_folder("//home/cloud_analytics/dwh/raw/crm/billingaccounts")[-1]
    email_leads_keys = find_tables_in_hahn_folder("//home/cloud_analytics/dwh/raw/crm/email_addr_bean_rel")[-1]
    email_table = find_tables_in_hahn_folder("//home/cloud_analytics/dwh/raw/crm/email_addresses")[-1]



    # информация по аккаунтам из CRM, включая емейл, которые есть только там
    # ['billing_account_id', 'date_entered', 'date_qualified', 
    # 'account_name', 'first_name', 'last_name', 'title', 'website', 
    # 'phone_mobile', 'email']

    req = f"""
            SELECT
                billing_account_id,
                date_entered,
                date_qualified,
                account_name,
                first_name,
                last_name,
                title,
                website,
                phone_mobile,
                email
            FROM (
                SELECT
                    id,
                    billing_account_id,
                    date_entered,
                    date_qualified,
                    account_name,
                    first_name,
                    last_name,
                    title,
                    website,
                    phone_mobile
                FROM (
                        SELECT
                            id,
                            toDate(date_entered) as date_entered,
                            toDate(date_modified) as date_qualified,
                            status,
                            first_name,
                            last_name,
                            title,
                            website,
                            phone_mobile,
                            account_name
                        FROM "{leads_table}"
                        WHERE lead_source == 'var'
                        and status = 'Converted'
                    ) AS a
                INNER JOIN (
                            SELECT
                                DISTINCT
                                leads_id,
                                billing_account_id
                            FROM "{billing_leads_keys}" as a
                            INNER JOIN (
                                        SELECT
                                            id,
                                            ba_id as billing_account_id
                                        FROM "{billing_table}"
                                    ) AS b ON a.billingaccounts_id == b.id
                            ) AS b ON a.id == b.leads_id
                  ) AS a
            LEFT JOIN (
                        SELECT
                            DISTINCT 
                            email_address as email,
                            id
                        FROM "{leads_table}" AS a
                        INNER JOIN (
                                    SELECT
                                        email_address,
                                        bean_id
                                    FROM "{email_leads_keys}" AS a
                                    INNER JOIN (
                                        SELECT
                                            email_address,
                                            id
                                        FROM "{email_table}"
                                    ) AS b ON a.email_address_id == b.id
                                 ) AS b ON a.id == b.bean_id
                     ) AS b ON a.id == b.id
            """

    crm_df = ch_adapter.execute_query(req, to_pandas=True)

    # Эти данные размеченную вручную информацию об аккаунтах, и в других местах её нет
    # ['billing_account_id', 'date_entered', 'date_qualified', 
    #  'account_name', 'first_name', 'last_name', 'title', 'website',  
    #  'phone_mobile', 'description', 'profile', 'email']


    req = """
            SELECT *
            FROM "//home/cloud_analytics/lunin-dv/additional_tickets_information/CLOUDANA-781"
          """
    prev_df = ch_adapter.execute_query(req, to_pandas=True)

    # оставляем только то, чего нет с crm_df
    add_df = prev_df[~prev_df["billing_account_id"].isin(crm_df["billing_account_id"])]


    # Загружаем cube_add_df -  пользователи довабились в кубик в обход CRM
    # ['billing_account_id', 'date_entered', 'date_qualified', 
    # 'account_name', 'first_name', 'last_name', 'title', 'website', 
    # 'phone_mobile', 'email'

    req = """
            SELECT
                billing_account_id,
                null as date_entered,
                date_qualified,
                account_name,
                first_name,
                last_name,
                null as title,
                null as website,
                toString(phone) as phone_mobile,
                user_settings_email as email
            FROM "//home/cloud_analytics/cubes/acquisition_cube/cube" as cube
            INNER JOIN (
                SELECT 
                    billing_account_id,
                    toDateTime(
                            min(if(replaceRegexpOne(feature_flags, '.*"var\": ([^,|}]*)[,|}].*', '\\1') like '%true%',
                            updated_at, null)
                            )
                    ) as date_qualified
                FROM "//home/cloud/billing/exported-billing-tables/billing_accounts_history_prod"
                GROUP BY billing_account_id
            ) AS dt ON cube.billing_account_id == dt.billing_account_id
            WHERE 
                isNotNull(billing_account_id)
                AND is_var == 'var'
                AND event == 'ba_created'
                AND billing_account_id != ''
                AND puid != ''
                AND master_account_id == ''
            """

    cube_add_df = ch_adapter.execute_query(req, to_pandas=True)


    cube_add_df.phone_mobile.fillna('-', inplace=True)
    cube_add_df.phone_mobile = cube_add_df.phone_mobile.apply(lambda x: str(int(x)) if x!='-' else x)

    # оставляем только то, чего нет с crm_df и add_df
    cube_add_df = cube_add_df[~cube_add_df["billing_account_id"].isin(crm_df["billing_account_id"])]
    cube_add_df = cube_add_df[~cube_add_df["billing_account_id"].isin(add_df["billing_account_id"])]


    # Объединяем три таблицы  crm_df, add_df и cube_add_df в одну таблицу : crm_df.
    # ['billing_account_id', 'date_entered', 'date_qualified',
    # 'account_name', 'first_name', 'last_name', 'title', 'website'
    # 'phone_mobile', 'email',
    # 'description', 'profile']

    def concatenate_tables(df_array):
        """Concatenates array of dataframes with possible Nan inside array
        :param df_array: array of dataframes
        :return: Nane or dataframe
        """
        df_array = [df for df in df_array if df is not None]
        if len(df_array) == 0:
            return None
        res_df = pd.concat(df_array, axis=0)
        res_df.reset_index(drop=True, inplace=True)

        return res_df

    crm_df = concatenate_tables([crm_df, add_df[crm_df.columns], cube_add_df[crm_df.columns]])

    # хотим сохранить информацию из поля  "profile", если она там была
    crm_df = pd.merge(crm_df, 
                      prev_df[["billing_account_id", "description", 'profile']], 
                      how='left')

    str_columns = ['first_name', 'last_name','title', 'website','phone_mobile', 'email', 'description','profile']
    crm_df[str_columns] = crm_df[str_columns].fillna('-')


    crm_df.date_entered.fillna('no_data', inplace=True)
    crm_df.date_entered = crm_df.date_entered.apply(lambda x : x[0:10])
    crm_df.date_qualified = crm_df.date_qualified.apply(lambda x : x[0:10])

    # Рассчитываем новые столбцы в таблице crm_df
    # Считаем:

    #     сколько дней прошло между датой entered и qualified    
    #     сколько дней прошло между датой qualified и now()    
    #     сколько недель/месяцев прошло между датой qualified и now()

    # crm_df = crm_df + 
    # ['num_of_days_from_entered_to_qualified',
    # 'days_to_now', 'weeks_to_now', 'months_to_now']


    crm_df["num_of_days_from_entered_to_qualified"] = crm_df[['date_entered', 'date_qualified']].apply(
        lambda row: make_num_of_days_from_entered_to_qualified(row), axis=1)

    crm_df["days_to_now"] = crm_df['date_qualified'].apply(lambda x: make_days_to_now(x))

    crm_df["weeks_to_now"] = crm_df["days_to_now"].apply(lambda x: make_weeks(x))
    crm_df["months_to_now"] = crm_df["days_to_now"].apply(lambda x: make_months(x))
    
    
    # Загружаем roles_df - информация  из CRM по  менеджерам, работающим с клиентами

    # ['billing_account_id',
    # 'architect', 'bus_dev', 'partner_manager', 'sales']


    accounts_table = find_tables_in_hahn_folder("//home/cloud_analytics/dwh/raw/crm/accounts")[-1]
    accountrole_table = find_tables_in_hahn_folder("//home/cloud_analytics/dwh/raw/crm/accountroles")[-1]
    billing_table = find_tables_in_hahn_folder("//home/cloud_analytics/dwh/raw/crm/billingaccounts")[-1]
    users_table = find_tables_in_hahn_folder("//home/cloud_analytics/dwh/raw/crm/users")[-1]

    roles = ['partner_manager','sales','bus_dev', 'architect']

    query = f"""
            SELECT DISTINCT
                ba.name AS billing_account_id,
                roles.role AS manager_role,
                u.user_name AS partner_manager,
                roles.name AS manager_name
            FROM "{accounts_table}" AS acc
            LEFT JOIN (select * from "{accountrole_table}") AS roles ON acc.id = roles.account_id
            LEFT JOIN (select * from "{billing_table}") AS ba ON acc.id = ba.account_id
            LEFT JOIN (select * from "{users_table}") AS u  ON u.id = roles.assigned_user_id
            WHERE
                --acc.deleted =0
                 billing_account_id is not null
                AND roles.role IN ({','.join(["'" + x + "'" for x in roles])} )
            """

    roles_df = ch_adapter.execute_query(query, to_pandas=True)

    roles_df.manager_name.fillna('-', inplace=True)
    roles_df.manager_name = roles_df.manager_name.apply(lambda x: x.split(' - ')[0] )

    # группируем имена менеджеров, если у них одна и та же роль
    roles_df = roles_df.groupby(['billing_account_id','manager_role'],
                              as_index=False).manager_name.agg(lambda x: '; '.join(x))

    # делаем так, чтобы роли - значения в колонке стали  именами колонки
    roles_df = roles_df.pivot(index="billing_account_id", columns='manager_role',values="manager_name").reset_index()

    roles_df.columns.name = None
    roles_df.fillna('-', inplace=True)
    
    # Мерджим crm_df и roles_df

    # crm_df = crm_df + 
    # ['architect', 'bus_dev', 'partner_manager', 'sales']
    
    crm_df = pd.merge(crm_df, roles_df, on='billing_account_id', how='left')
    crm_df[roles] = crm_df[roles].fillna('-')
    
    return crm_df
    
def get_mail_subscription(ch_adapter):

    #  Загружаем из кубика информацию о настройках подписки VAR-аккаунтов

    # ['billing_account_id', 'billing_created_time', 
    #  'mail_billing','mail_tech','mail_testing', 'mail_feature', 'mail_promo', 'mail_info', 'mail_event']

    logger.info('Start get_mail_subscription')

    req = f"""
            SELECT 
                billing_account_id,
                toDate(event_time) AS billing_created_time,
                mail_billing,
                mail_tech,
                mail_testing,
                mail_feature,
                mail_promo,
                mail_info,
                mail_event
            FROM "//home/cloud_analytics/cubes/acquisition_cube/cube"
            WHERE 
                isNotNull(billing_account_id)
                AND is_var == 'var'
                AND event == 'ba_created'
                AND billing_account_id != ''
                AND puid != ''
                AND master_account_id == ''
             """
    df = ch_adapter.execute_query(req, to_pandas=True)
    
    return df

def get_puid_bo_contacts(ch_adapter):

    # Загружаем информацию по puid, backoffice_email, backoffice_phone, backoffice_name

    # ['billing_account_id', 
    # 'puid', 
    # 'backoffice_email', 'backoffice_phone', 'backoffice_name']

    logger.info('Start get_puid_bo_contacts')


    req = """
            SELECT *
            FROM (
                    SELECT 
                        billing_account_id,
                        assumeNotNull(puid) AS puid
                    FROM "//home/cloud_analytics/cubes/acquisition_cube/cube"
                    WHERE 
                        isNotNull(billing_account_id)
                        AND is_var == 'var'
                        AND event == 'ba_created'
                        AND billing_account_id != ''
                        AND puid != ''
                        AND master_account_id == ''
                ) AS cube_info
            INNER JOIN (
                        SELECT
                            CAST(assumeNotNull(passport_id) as String) AS puid,
                            argMax(email, dt) AS backoffice_email,
                            argMax(phone, dt) AS backoffice_phone,
                            argMax(name, dt) AS backoffice_name
                        FROM "//home/cloud_analytics/import/balance/balance_persons"
                        WHERE 
                            puid != ''
                        GROUP BY 
                            puid
                    ) AS backoiffice_info
            USING puid
            """

    df = ch_adapter.execute_query(req, to_pandas=True)
    df["puid"] = df["puid"].astype(str)
    
    return df

def get_invited_subaccounts(ch_adapter):
    #  Загружаем количество приведенных сабаккаунтов за 1-ый ,2-ой ,3,4,5,6 -ой  месяц от  date_quailfied 
    # и за все время:
    # ['billing_account_id', 
    # 'one_month_sub_accounts', 'two_month_sub_accounts', 'three_month_sub_accounts', 'four_month_sub_accounts', 
    # 'five_month_sub_accounts', 'six_month_sub_accounts', 'six_plus_month_sub_accounts', 'all_sub_accounts_number']
    
    logger.info('Start get_invited_subaccounts')

    leads_table = find_tables_in_hahn_folder("//home/cloud_analytics/dwh/raw/crm/leads")[-1]
    billing_leads_keys = find_tables_in_hahn_folder("//home/cloud_analytics/dwh/raw/crm/leads_billing_accounts")[-1]
    billing_table = find_tables_in_hahn_folder("//home/cloud_analytics/dwh/raw/crm/billingaccounts")[-1]
    
    req = f"""
    SELECT
        master_account_id AS billing_account_id,
        sum_one_month_sub_accounts AS one_month_sub_accounts,
        sum_two_month_sub_accounts - sum_one_month_sub_accounts AS two_month_sub_accounts,
        sum_three_month_sub_accounts - sum_two_month_sub_accounts AS three_month_sub_accounts,
        sum_four_month_sub_accounts - sum_three_month_sub_accounts AS four_month_sub_accounts,
        sum_five_month_sub_accounts - sum_four_month_sub_accounts AS five_month_sub_accounts,
        sum_six_month_sub_accounts - sum_five_month_sub_accounts AS six_month_sub_accounts,
        sum_six_plus_month_sub_accounts - sum_six_month_sub_accounts AS six_plus_month_sub_accounts,
        sum_six_plus_month_sub_accounts AS all_sub_accounts_number
    FROM (
        SELECT
            master_account_id,
            length(groupUniqArray(IF(toDate(event_time) < addDays(toDate(date_qualified), 30), b.billing_account_id, null))) AS sum_one_month_sub_accounts,
            length(groupUniqArray(IF(toDate(event_time) < addDays(toDate(date_qualified), 60), b.billing_account_id, null))) AS sum_two_month_sub_accounts,
            length(groupUniqArray(IF(toDate(event_time) < addDays(toDate(date_qualified), 90), b.billing_account_id, null))) AS sum_three_month_sub_accounts,
            length(groupUniqArray(IF(toDate(event_time) < addDays(toDate(date_qualified), 4 * 30), b.billing_account_id, null))) AS sum_four_month_sub_accounts,
            length(groupUniqArray(IF(toDate(event_time) < addDays(toDate(date_qualified), 5 * 30), b.billing_account_id, null))) AS sum_five_month_sub_accounts,
            length(groupUniqArray(IF(toDate(event_time) < addDays(toDate(date_qualified), 6 * 30), b.billing_account_id, null))) AS sum_six_month_sub_accounts,
            length(groupUniqArray(billing_account_id)) AS sum_six_plus_month_sub_accounts
        FROM "//home/cloud_analytics/cubes/acquisition_cube/cube" AS a
        INNER JOIN (
            SELECT
                billing_account_id,
                date_qualified
            FROM (
                SELECT
                    billing_account_id,
                    ba_time,
                    if (isNotNull(date_qualified), date_qualified, ba_time) AS date_qualified
                FROM (
                    SELECT 
                        billing_account_id,
                        toDate(event_time) AS ba_time
                    FROM "//home/cloud_analytics/cubes/acquisition_cube/cube"
                    WHERE 
                        isNotNull(billing_account_id)
                        AND is_var == 'var'
                        AND event == 'ba_created'
                        AND billing_account_id != ''
                        AND puid != ''
                        AND master_account_id == ''
                ) AS a
                LEFT JOIN (
                    SELECT
                        billing_account_id,
                        date_qualified
                    FROM (
                        SELECT
                            id,
                            toDate(date_modified) AS date_qualified
                        FROM "{leads_table}"
                        WHERE 
                            lead_source == 'var'
                            AND status = 'Converted'
                    ) AS a
                    INNER JOIN (
                        SELECT
                            DISTINCT
                            leads_id,
                            billing_account_id
                        FROM "{billing_leads_keys}" AS a
                        INNER JOIN (
                            SELECT
                                id,
                                ba_id AS billing_account_id
                            FROM "{billing_table}"
                        ) AS b
                        ON a.billingaccounts_id == b.id
                    ) AS b ON a.id == b.leads_id
                ) AS b ON a.billing_account_id == b.billing_account_id
            )
        ) AS b ON a.master_account_id == b.billing_account_id
        WHERE 
            billing_account_id != ''
        GROUP BY 
            master_account_id
    )
    """    
    
    df = ch_adapter.execute_query(req, to_pandas=True)
    
    return df

def get_ba_consumption(ch_adapter, col_names, intervals):
    #  поквартальное потребление по каждому биллинг-аккаунту самого вар-клиента

    # ['billing_consumption_2020_Q4', 'billing_consumption_2020_Q3', 'billing_consumption_2020_Q2',
    # 'billing_consumption_2020_Q1', 'billing_consumption_2019_Q4',
    # 'billing_account_id']

    logger.info('Start get_ba_consumption')


    part_req = ""

    for col, (left, right) in zip(col_names, iterate_by_two(intervals)):
        part_req += f"""    SUM(if(toDate(event_time) >= {left} 
        AND toDate(event_time) < {right}, 
        real_consumption_vat, 0)) AS billing_consumption_{col},\n\n"""

    req = f"""
            SELECT
                {part_req}
                billing_account_id
            FROM "//home/cloud_analytics/cubes/acquisition_cube/cube"
            WHERE 
                billing_account_id != ''
                AND event='day_use'
                AND ba_usage_status != 'service' 
                AND is_var = 'var'
            GROUP BY billing_account_id
            """

    df = ch_adapter.execute_query(req, to_pandas=True)
    
    return df

def get_sub_consumption(ch_adapter, col_names, intervals):
    
    # Поквартальное потребление    тех, кого привел вар-клиент, привязанное к нему

    # ['cons_subaccount_2020_Q4', 'subaccount_num_2020_Q4', 'avg_cons_by_one_subaccount_2020_Q4',
    # 'cons_subaccount_2020_Q3', 'subaccount_num_2020_Q3', 'avg_cons_by_one_subaccount_2020_Q3',
    # 'cons_subaccount_2020_Q2', 'subaccount_num_2020_Q2', 'avg_cons_by_one_subaccount_2020_Q2',
    # 'cons_subaccount_2020_Q1', 'subaccount_num_2020_Q1', 'avg_cons_by_one_subaccount_2020_Q1',
    # 'cons_subaccount_2019_Q4', 'subaccount_num_2019_Q4', 'avg_cons_by_one_subaccount_2019_Q4', 
    # 'billing_account_id']

    logger.info('Start get_sub_consumption')

    part_req = ""
    for col, (left, right) in zip(col_names, iterate_by_two(intervals)):
        part_req += f"""    SUM(if(toDate(event_time) >= {left} 
        and toDate(event_time) < {right}, 
        real_consumption_vat, 0)) AS cons_subaccount_{col},
        length(groupUniqArray(if(toDate(event_time) >= {left} 
        and toDate(event_time) < {right}, 
        billing_account_id, null))) AS subaccount_num_{col},
        cons_subaccount_{col} / (subaccount_num_{col} + 1e-5)
        AS avg_cons_by_one_subaccount_{col},
        \n\n"""

    req = f"""
            SELECT
                {part_req}
                master_account_id
            FROM "//home/cloud_analytics/cubes/acquisition_cube/cube"
            WHERE  
                master_account_id != ''
                AND isNotNull(master_account_id)
                AND event = 'day_use'           
            GROUP BY master_account_id
         """

    df = ch_adapter.execute_query(req, to_pandas=True)

    df.rename(columns={'master_account_id':'billing_account_id'}, inplace=True)
    
    return df

def save_table(file_to_write, path, table, schema=None, append=False):
    assert (path[-1] != '/')
    df = table.copy()
    real_schema = apply_type(schema, df)
    json_df_str = df.to_json(orient='records')
    path = path + "/" + file_to_write
    json_df = json.loads(json_df_str)
    if not yt.exists(path) or not append:
        yt.create(type="table", path=path, force=True,
                  attributes={"schema": real_schema})
    tablepath = yt.TablePath(path, append=append)
    yt.write_table(tablepath, json_df,
                   format=yt.JsonFormat(attributes={"encode_utf8": False}))
    
def apply_type(raw_schema, df):
    if raw_schema is not None:
        for key in raw_schema:
            if 'list:' not in str(raw_schema[key]) and raw_schema[key] != 'datetime':
                df[key] = df[key].astype(raw_schema[key])
            if raw_schema[key] == 'datetime':
                df[key] = df[key].apply(lambda x: time_to_unix(x))
    schema = []
    for col in df.columns:
        if raw_schema is not None and raw_schema.get(col) is not None and raw_schema[col] == 'datetime':
            schema.append({"name": col, 'type': 'datetime'})
            continue
        if df[col].dtype == int:
            schema.append({"name": col, 'type': 'int64'})
        elif df[col].dtype == float:
            schema.append({"name": col, 'type': 'double'})
        elif raw_schema is not None and raw_schema.get(col) is not None and 'list:' in str(raw_schema[col]):
            second_type = raw_schema[col].split("list:")[-1]
            schema.append({"name": col, 'type_v3':
                {"type_name": 'list', "item": {"type_name": "optional", "item": second_type}}})
        else:
            schema.append({"name": col, 'type': 'string'})
    return schema



if __name__ == "__main__":
    main()