import json

import requests
import yt.wrapper as yt


def get_from_staff(logins, staff_token, staff_url):
    params = {
        "login": ','.join(logins),
        "_fields": "login,work_email,official.position,name.first,name.last"
    }

    r = requests.get(staff_url, headers={'Authorization': 'OAuth ' + staff_token}, params=params)

    staff = json.loads(r.text)
    return {row["login"]: row for row in staff['result']}


def enrich_sales_name_table(src_table_path, dst_table_path, yt_token, yt_proxy, staff_token, staff_url):
    yt.config['token'] = yt_token
    yt.config['proxy']['url'] = yt_proxy

    src_tp = yt.TablePath(src_table_path)
    dst_tp = yt.TablePath(dst_table_path)

    sales_list = list(yt.read_table(src_tp, format=yt.JsonFormat()))

    distinct_logins = {row['sales_name'] for row in sales_list if row['sales_name'] != ''}

    data_by_login = get_from_staff(distinct_logins, staff_token, staff_url)

    result = [
        {'billing_account_id': row['billing_account_id'],
         'login': row['sales_name'],
         'first_name_ru': data_by_login[row['sales_name']]['name']['first']['ru'] if row['sales_name'] else '',
         'first_name_en': data_by_login[row['sales_name']]['name']['first']['en'] if row['sales_name'] else '',
         'last_name_ru': data_by_login[row['sales_name']]['name']['last']['ru'] if row['sales_name'] else '',
         'last_name_en': data_by_login[row['sales_name']]['name']['last']['en'] if row['sales_name'] else '',
         'position_ru': data_by_login[row['sales_name']]['official']['position']['ru'] if row['sales_name'] else '',
         'position_en': data_by_login[row['sales_name']]['official']['position']['en'] if row['sales_name'] else '',
         'work_email': data_by_login[row['sales_name']]['work_email'] if row['sales_name'] else ''}
        for row in sales_list
    ]
    yt.write_table(
        dst_tp,
        '\n'.join(map(json.dumps, result)).encode("utf-8"),
        raw=True,
        format=yt.JsonFormat(attributes={"encode_utf8": False})
    )
