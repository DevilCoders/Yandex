#!/usr/bin/env python2
# -*- coding: utf-8 -*-
import pandas as pd, datetime, ast, numpy as np, telebot, json, requests, os, sys, pymysql,time
module_path = os.path.abspath(os.path.join('/home/ktereshin/yandex/arcadia/cloud/analytics/python/work'))
if module_path not in sys.path:
    sys.path.append(module_path)
module_path = os.path.abspath(os.path.join('/home/ktereshin/yandex/cron'))
if module_path not in sys.path:
    sys.path.append(module_path)
from nile.api.v1 import (
    clusters,
    aggregators as na,
    extractors as ne,
    filters as nf,
    Record
)
from creds import (
    yt_creds,
    metrika_creds,
    yc_ch_creds,
    crm_sql_creds,
    stat_creds,
    telebot_creds
)

from acquisition_cube_init_funnel_steps import (
    paths_dict_prod,
    paths_dict_test
)

def delete_table(path_, tables_dir_, cluster_yt_, job_):
    if path_ in get_table_list(tables_dir_, job_)[1:-1].split(','):
        counter = 0
        while True:
            time.sleep(2)
            try:
                cluster_yt_.driver.remove(path_)
                break
            except Exception as err:
                print(err)
                counter += 1
                if counter == 15:
                    break

def convert_metric_to_float(num):
    try:
        return float(num)
    except:
        return 0.0


def get_last_not_empty_table(folder_path, job):
    tables_list = sorted([folder_path + '/' + x for x in job.driver.list(folder_path)], reverse=True)
    last_table_rows = 0
    last_table = ''
    for table in tables_list:
        try:
            table_rows = int(job.driver.get_attribute(table, 'chunk_row_count'))
        except:
            continue

        if table_rows > last_table_rows:
            last_table_rows =  table_rows
            last_table = table
    if last_table:
        return last_table
    else:
        return tables_list[0]


def get_table_list(folder_path, job):
    tables_list = sorted([folder_path + '/' + x for x in job.driver.list(folder_path)], reverse=True)
    return '{%s}' % (','.join(tables_list))

def apply_types_in_project(schema_):
    apply_types_dict = {}
    for col in schema_:

        if schema_[col] == str:
            apply_types_dict[col] = ne.custom(lambda x: str(x).replace('"', '').replace("'", '').replace('\\','') if x not in ['', None] else None, col)

        elif schema_[col] == int:
            apply_types_dict[col] = ne.custom(lambda x: int(x) if x not in ['', None] else None, col)

        elif schema_[col] == float:
            apply_types_dict[col] = ne.custom(lambda x: float(x) if x not in ['', None] else None, col)
    return apply_types_dict

def execute_query(query, cluster, alias, token, timeout=1200):
    proxy = "http://{}.yt.yandex.net".format(cluster)
    s = requests.Session()
    url = "{proxy}/query?database={alias}&password={token}".format(proxy=proxy, alias=alias, token=token)
    resp = s.post(url, data=query, timeout=timeout)
    resp.raise_for_status()
    rows = resp.content.strip().split('\n')
    return rows

def chyt_execute_query(
    query,
    cluster,
    alias,
    token,
    columns,
    create_table_dict = {}
):
    i = 0
    while True:
        try:
            if 'create table' in query.lower() or 'insert into' in query.lower():
                if create_table_dict:
                    delete_table(
                        create_table_dict['path'],
                        create_table_dict['tables_dir'],
                        create_table_dict['cluster'],
                        create_table_dict['cluster'].job()
                    )
                    execute_query(query=query, cluster=cluster, alias=alias, token=token)
                else:
                    execute_query(query=query, cluster=cluster, alias=alias, token=token)
                return 'Success!!!'
            else:
                result = execute_query(query=query, cluster=cluster, alias=alias, token=token)
                users = pd.DataFrame([row.split('\t') for row in result], columns = columns)
                return users
        except Exception as err:
            if 'IncompleteRead' in  str(err.message):
                return 'Success!!!'
            print(err.message)
            i += 1
            time.sleep(5)
            if i > 30:
                print(query)
                raise ValueError('Bad Query!!!')

def delete_table(path_, tables_dir_, cluster_yt_, job_):
    if path_ in get_table_list(tables_dir_, job_)[1:-1].split(','):
        counter = 0
        while True:
            time.sleep(2)
            try:
                cluster_yt_.driver.remove(path_)
                break
            except Exception as err:
                print(err)
                counter += 1
                if counter == 15:
                    break

def main():
    cluster = 'hahn'
    alias = "*cloud_analytics"
    #alias = "*ch_public"
    token = '%s' % (yt_creds['value']['token'])

    mode = 'prod'
    bot = telebot.TeleBot(telebot_creds['value']['token'])
    cluster_yt = clusters.yt.Hahn(
        token = yt_creds['value']['token'],
        pool = yt_creds['value']['pool']
    )
    if mode == 'test':
        paths_dict_temp = paths_dict_test
        folder = 'cloud_analytics_test'
        tables_dir = "//home/{0}/cooking_cubes/acquisition_cube/sources".format(folder)
    elif mode == 'prod':
        paths_dict_temp = paths_dict_prod
        folder = 'cloud_analytics'
        tables_dir = "//home/{0}/cooking_cubes/acquisition_cube/sources".format(folder)
    paths_dict = paths_dict_temp.copy()

    job = cluster_yt.job()
    #for path in['billing_accounts_history_path', 'billing_accounts_path']:
        #valid_path = get_last_not_empty_table(paths_dict_temp[path], job)
        #paths_dict[path] = valid_path
    fraud_path = get_last_not_empty_table('//home/antifraud/export/cloud/bad_accounts/1h', job)
    offers_path = '//home/cloud/billing/exported-billing-tables/monetary_grant_offers_prod'
    grants_path = '//home/cloud/billing/exported-billing-tables/monetary_grants_prod'

    query = '''
    SELECT
        DISTINCT
        login
    FROM "//home/cloud_analytics/import/staff/cloud_staff/cloud_staff_history"
    WHERE
        group_name IN ('Группа по привлечению новых клиентов', 'Группа аналитики Яндекс.Облака')
    '''
    path = "//home/{0}/import/crm/roles/data_".format(folder)

    mass_logins = chyt_execute_query(
        query=query,
        cluster=cluster,
        alias=alias,
        token=token,
        columns = ['login'],
        create_table_dict = {
            'path': path,
            'tables_dir': tables_dir,
            'cluster': cluster_yt
        }
    )['login'].to_list() + ['admin', '']

    query = '''
        SELECT
            DISTINCT
            ba.ba_id as billing_account_id,
            CAST(DATE_ADD(date_created, INTERVAL -3 HOUR) AS CHAR) as time,
            IFNULL(l1.user_name,NULL) user_name,
            'main_sales' as role,
            CASE
                WHEN field_name = 'assigned_user_share' AND after_value_string > 0 THEN after_value_string
                WHEN field_name = 'assigned_user_share' AND after_value_string <= 0 THEN 100
                ELSE NULL
            END AS share,
            field_name as event
        FROM accounts_audit as acc_hist
        LEFT JOIN users l1
        ON acc_hist.after_value_string=l1.id AND l1.deleted=0
        LEFT JOIN accounts as acc
        ON acc_hist.parent_id=acc.id
        LEFT JOIN billingaccounts as ba
        ON acc_hist.parent_id=ba.account_id
        LEFT JOIN team_sets
        ON acc.team_set_id = team_sets.id
        LEFT JOIN team_sets_teams
        ON team_sets.id = team_sets_teams.team_set_id
        LEFT JOIN teams
        ON team_sets_teams.team_id = teams.id
        LEFT JOIN users as acc_t
        ON acc_t.id = teams.associated_user_id
        WHERE
            field_name IN ('assigned_user_id', 'assigned_user_share')
            AND ba.ba_id IN ('dn2ejf3i4p8o6b47e05l')
        UNION ALL
        SELECT
            DISTINCT
            ba.ba_id as billing_account_id,
            CAST(DATE_ADD(acc.date_entered, INTERVAL -3 HOUR) AS CHAR) as time,
            IFNULL(l1.user_name,'') user_name,
            'main_sales' as role,
            CASE WHEN acc.assigned_user_share > 0 THEN acc.assigned_user_share ELSE 100 END AS share,
            'account_created' as event
        FROM accounts as acc
        LEFT JOIN users l1
        ON acc.assigned_user_id=l1.id AND l1.deleted=0
        LEFT JOIN billingaccounts as ba
        ON acc.id=ba.account_id
        LEFT JOIN team_sets
        ON acc.team_set_id = team_sets.id
        LEFT JOIN team_sets_teams
        ON team_sets.id = team_sets_teams.team_set_id
        LEFT JOIN teams
        ON team_sets_teams.team_id = teams.id
        LEFT JOIN users as acc_t
        ON acc_t.id = teams.associated_user_id
        WHERE
            ba.ba_id IN ('dn2ejf3i4p8o6b47e05l')
        UNION ALL
        SELECT
            ba.ba_id as billing_account_id,
            CAST(DATE_ADD(r.date_entered, INTERVAL -3 HOUR) AS CHAR) as time,
            u.user_name,
            r.role,
            CASE WHEN r.share > 0 THEN r.share ELSE 0 END as share,
            'role' as event
        FROM accounts AS acc
        LEFT JOIN accountroles AS r
            ON acc.id = r.account_id
        LEFT JOIN users AS u
            on u.id = r.assigned_user_id
        LEFT JOIN billingaccounts as ba
            ON acc.id=ba.account_id
        WHERE
            r.role IS NOT NULL
            AND ba.ba_id IN ('dn2ejf3i4p8o6b47e05l')
        UNION ALL
        SELECT
            ba.ba_id as billing_account_id,
            CAST(DATE_ADD(r.date_modified, INTERVAL -3 HOUR) AS CHAR) as time,
            u.user_name,
            r.role,
            CASE WHEN r.share > 0 THEN r.share ELSE 0 END as share,
            'last_role' as event
        FROM accounts AS acc
        LEFT JOIN accountroles AS r
            ON acc.id = r.account_id
        LEFT JOIN users AS u
            on u.id = r.assigned_user_id
        LEFT JOIN billingaccounts as ba
            ON acc.id=ba.account_id
        WHERE
            ba.ba_id IN ('dn2ejf3i4p8o6b47e05l')
    '''

    query = '''
    SELECT
        DISTINCT
        billing_account_id,
        time,
        user_name,
        role,
        share
    FROM(
        SELECT
            DISTINCT
            ba.ba_id as billing_account_id,
            CAST(DATE_ADD(date_created, INTERVAL -3 HOUR) AS CHAR) as time,
            IFNULL(l1.user_name,'') user_name,
            'main_sales' as role,
            100 AS share
        FROM accounts_audit as acc_hist
        LEFT JOIN users l1
        ON acc_hist.after_value_string=l1.id AND l1.deleted=0
        LEFT JOIN accounts as acc
        ON acc_hist.parent_id=acc.id
        LEFT JOIN billingaccounts as ba
        ON acc_hist.parent_id=ba.account_id
        LEFT JOIN team_sets
        ON acc.team_set_id = team_sets.id
        LEFT JOIN team_sets_teams
        ON team_sets.id = team_sets_teams.team_set_id
        LEFT JOIN teams
        ON team_sets_teams.team_id = teams.id
        LEFT JOIN users as acc_t
        ON acc_t.id = teams.associated_user_id
        WHERE
            field_name = 'assigned_user_id'
            AND ba.ba_id IS NOT NULL
            AND l1.user_name NOT IN ({0})
        UNION ALL
        SELECT
            DISTINCT
            ba.ba_id as billing_account_id,
            CAST(DATE_ADD(acc.date_entered, INTERVAL -3 HOUR) AS CHAR) as time,
            IFNULL(l1.user_name,'') user_name,
            'main_sales' as role,
            CASE WHEN acc.assigned_user_share > 0 THEN acc.assigned_user_share ELSE 100 END AS share
        FROM accounts as acc
        LEFT JOIN users l1
        ON acc.assigned_user_id=l1.id AND l1.deleted=0
        LEFT JOIN billingaccounts as ba
        ON acc.id=ba.account_id
        LEFT JOIN team_sets
        ON acc.team_set_id = team_sets.id
        LEFT JOIN team_sets_teams
        ON team_sets.id = team_sets_teams.team_set_id
        LEFT JOIN teams
        ON team_sets_teams.team_id = teams.id
        LEFT JOIN users as acc_t
        ON acc_t.id = teams.associated_user_id
        WHERE
            ba.ba_id IS NOT NULL
            AND l1.user_name NOT IN ({0})
        UNION ALL
        SELECT
            ba.ba_id as billing_account_id,
            CAST(DATE_ADD(r.date_entered, INTERVAL -3 HOUR) AS CHAR) as time,
            u.user_name,
            IF(r.role_custom_audit IS NULL OR r.role_custom_audit = '', r.role, r.role_custom_audit) as role_custom_audit,
            CASE
                WHEN r.share_custom_audit > 0 THEN r.share_custom_audit
                WHEN r.share > 0 THEN r.share
            ELSE 0 END as share
        FROM accounts AS acc
        LEFT JOIN accountroles AS r
            ON acc.id = r.account_id
        LEFT JOIN users AS u
            on u.id = r.assigned_user_id
        LEFT JOIN billingaccounts as ba
            ON acc.id=ba.account_id
        WHERE
            r.role IS NOT NULL
            AND ba.ba_id IS NOT NULL
            AND u.user_name NOT IN ({0})

        UNION ALL
        SELECT
            DISTINCT
            billing_account_id,
            time,
            user_name,
            role,
            share
        FROM(
            SELECT
                ba.ba_id as billing_account_id,
                IF(acc_hist.event_id = 'ff5fc870-78a3-11ea-bd57-f580ebdba5f3', 'f8f61052-78a3-11ea-ac3e-9ac54526f1cf', acc_hist.event_id) as event_id_,
                CAST(DATE_ADD(IF(acc_hist.event_id = 'ff5fc870-78a3-11ea-bd57-f580ebdba5f3', CAST('2020-04-07 07:47:03' AS Datetime), date_created), INTERVAL -3 HOUR) AS CHAR) as time,
                acc.id as account_id,
                MAX(IF(field_name = 'assigned_user_id_custom_audit' OR field_name = 'assigned_user_id', user_name, '')) as user_name,
                MAX(IF(field_name = 'share_custom_audit' OR field_name = 'share', after_value_string, '')) as share,
                MAX(IF(field_name = 'role_custom_audit' OR field_name = 'role', after_value_string, 100)) AS role
            FROM accountroles_audit as acc_hist
            LEFT JOIN accountroles as roles
                ON acc_hist.parent_id = roles.id
            LEFT JOIN accounts as acc
                ON acc.id = roles.account_id
            LEFT JOIN billingaccounts as ba
                ON ba.account_id = acc.id
            LEFT JOIN users AS u
                ON u.id = acc_hist.after_value_string
            WHERE
                ba.ba_id IS NOT NULL
            GROUP BY
                ba.ba_id,
                event_id_,
                time,
                account_id
        ) as t1
        WHERE
            user_name NOT IN ({0})
    ) as t0
    '''.format("'"+"','".join(mass_logins) + "'")

    cnx = pymysql.connect(
        user='cloud8',
        password=crm_sql_creds['value']['readonly'],
        host='man-ykvctt121sr8xnc5.db.yandex.net',
        port = 3306,
        database='cloud8'
    )

    account_changes = pd.read_sql_query(query, cnx)
    cnx.close()

    cluster_yt.write(path, account_changes)

    schema = {
        "billing_account_id": str,
        "time": str,
        "user_name": str,
        "role":str,
        "share": int
    }
    job = cluster_yt.job()
    job.table(path) \
    .project(
        **apply_types_in_project(schema)
    ) \
    .put(path[:-1], schema = schema)
    job.run()

    query = '''
    CREATE TABLE "//home/cloud_analytics/import/crm/roles/roles_daily" ENGINE = YtTable() AS
    SELECT
        billing_account_id,
        days_ as date,
        role,
        user_name,
        share
    FROM(
        SELECT
            t0.*,
            t1.date_next,
            arrayMap(x -> toString(addDays(toDate(t0.date), x)) ,range(assumeNotNull(toUInt32( t1.date_next -  t0.date    ))) ) as days_
        FROM(
            SELECT
                billing_account_id,
                toDate(time) as date,
                role,
                groupArray(user_name)[1] as user_name,
                groupArray(share)[1] as share
            FROM (SELECT * FROM "//home/cloud_analytics/import/crm/roles/data" ORDER BY time DESC)
            GROUP BY
                billing_account_id,
                date,
                role
            ORDER BY
                billing_account_id,
                date,
                role
        ) as t0
        ANY LEFT JOIN (
            SELECT
                billing_account_id,
                role,
                dates as date,
                dates_ as date_next
            FROM(
                SELECT
                    billing_account_id,
                    role,
                    groupArray(date) as dates,
                    arrayConcat(arraySlice(dates, 2,length(dates)), [addDays(toDate(NOW()), 1)]) as dates_
                FROM(
                    SELECT
                        DISTINCT
                        billing_account_id,
                        role,
                        dates as date
                    FROM(
                        SELECT
                            billing_account_id,
                            role,
                            groupArray(toDate(time)) as dates
                        FROM (SELECT * FROM "//home/cloud_analytics/import/crm/roles/data" ORDER BY time DESC)
                        GROUP BY
                            billing_account_id,
                            role
                    )
                    ARRAY JOIN dates
                    ORDER BY
                        date
                )
                GROUP BY
                    billing_account_id,
                    role
            )
            ARRAY JOIN dates, dates_
        ) as t1
        ON t0.billing_account_id = t1.billing_account_id AND t0.date = t1.date AND t0.role = t1.role
        ORDER BY
            billing_account_id,
            date
    )
    ARRAY JOIN days_
    '''
    path = "//home/cloud_analytics/import/crm/roles/roles_daily"

    chyt_execute_query(
        query=query,
        cluster=cluster,
        alias=alias,
        token=token,
        columns = [],
        create_table_dict = {
            'path': path,
            'tables_dir': '//home/cloud_analytics/import/crm/roles',
            'cluster': cluster_yt
        }
    )

if __name__ == '__main__':
    main()
