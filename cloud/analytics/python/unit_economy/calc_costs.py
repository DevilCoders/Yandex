import requests
from tokens import yt_token
from tokens import wiki_token
import pandas as pd
import numpy as np
from datetime import datetime

today = datetime.now().strftime('%Y-%m-%d')


HOME_FOLDER = '/home/artkaz/ya/cloud/analytics/python/unit_economy/'


headers = {
    'Authorization': 'OAuth ' + wiki_token,
    'Content-Type': 'application/json'
}

#support count

# url = 'https://staff-api.yandex-team.ru/v3/persons?_query=(department_group.department.id==3905%20and%20official.is_dismissed==false)&_fields=official.join_at,official.is_dismissed,login'
# r = requests.get(url=url, headers=headers)

# df = pd.DataFrame([[x['login'], x['official']['join_at']] for x in r.json()['result']], columns = ['login','join_at']).sort_values('join_at')
# df ['count'] = 1
# df['count'] = np.cumsum(df['count'])


# multi_if_condition_support = 'multiIf(' + ', '.join(['date<=\'' + r[1]['join_at'] + '\', ' + str(r[1]['count']-1) for r in df.iterrows()]) + ', ' + str(np.max(df['count'])) + ')'



#wages

# url = 'https://wiki-api.yandex-team.ru/_api/frontend/users/artkaz/Unit-Economy/wages/.grid'
# headers = {
#     'Authorization': 'OAuth ' + wiki_token,
#     'Content-Type': 'application/json'
# }

# r = requests.get(url=url, headers=headers)

# df = pd.DataFrame([[x['raw'] for x in x_] for x_ in r.json()['data']['rows']], columns=['type', 'wage'])

params = {
    'support': {'r1':'if(date<\'2020-03-10\', 1300, 1180)'},
}



def execute_query(query,
                  cluster= 'hahn',
                  alias = "*ch_public",
                  token = yt_token,
                  timeout=60000):
    #logger.info("Executing query: %s", query)
    proxy = "http://{}.yt.yandex.net".format(cluster)
    s = requests.Session()
    url = "{proxy}/query?database={alias}&password={token}".format(proxy=proxy, alias=alias, token=token)
    resp = s.post(url, data=query, timeout=timeout)
    resp.raise_for_status()
    rows = resp.content.strip()#.split('\n')
    #logger.info("Time spent: %s seconds, rows returned: %s", resp.elapsed.total_seconds(), len(rows))
    return rows
print (execute_query('SELECT now()'))

from tqdm import tqdm

costs_sql_files = {
    'ad': 'ad_costs.sql',
    # 'server': 'server_costs.sql',
    'support': 'support_costs.sql',
}

table_paths = {
    'ad': [     '//home/cloud_analytics/unit_economy/marketing_perfomance_costs/marketing_perfomance_costs',
                #'//home/cloud_analytics/unit_economy/marketing_perfomance_costs/history/'+today
                ],
    'server': [ '//home/cloud_analytics/unit_economy/hardware_costs/hardware_costs',
                '//home/cloud_analytics/unit_economy/hardware_costs/history/'+today],
    'support': [ '//home/cloud_analytics/unit_economy/support_costs/support_costs',
                #'//home/cloud_analytics/unit_economy/support_costs/history/'+today
               ],
}

costs_sql = {}

for k in costs_sql_files.keys():
    print (k)
    q = ''
    with open(HOME_FOLDER + costs_sql_files[k], 'r') as file:
        q = file.read()
    for t in table_paths[k]:
        print (t)
        q_create = """
            CREATE TABLE IF NOT EXISTS "{table}"
            (
            date String,
            costs_type String,
            costs Float64,
            billing_account_id String
            ) ENGINE = YtTable();
        """.format(table=t)
        execute_query(q_create)
        #print(q.format(table = t))
        if k in params:
            q_ = q.format(**{**params[k], **{'table':t}})
            # print (q_)
        else:
            q_ = q.format(table=t)
        print ('')
        # print (q_)
        execute_query(q_)
print('SUCCESS')
