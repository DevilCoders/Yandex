import requests
from tokens import yt_token
from tokens import wiki_token
import pandas as pd
import numpy as np
from tokens import wiki_token, yt_token
import asyncio, aiohttp
from tqdm import tqdm
import yt.wrapper
from datetime import datetime
today = datetime.now().strftime('%Y-%m-%d')

def execute_query_chyt(query,
                  cluster= 'hahn',
                  alias = "*ch_public",
                  token = yt_token,
                  timeout=600):
    #logger.info("Executing query: %s", query)
    proxy = "http://{}.yt.yandex.net".format(cluster)
    s = requests.Session()
    url = "{proxy}/query?database={alias}&password={token}".format(proxy=proxy, alias=alias, token=token)
    resp = s.post(url, data=query, timeout=timeout)
    if resp.status_code != 200:
        print("Response status: %s", resp.status_code)
        print("Response headers: %s", resp.headers)
        print("Response content: %s", resp.content)
    resp.raise_for_status()
    rows = resp.content.strip()#.split('\n')
    #logger.info("Time spent: %s seconds, rows returned: %s", resp.elapsed.total_seconds(), len(rows))
    return rows

print (execute_query_chyt('SELECT now()'))

async def download_persons():
    def make_params(min_id, department_url):
        return {
            '_sort': 'id',
            '_query': f'id > {min_id} and (group.url == "{department_url}" or group.ancestors.url == "{department_url}")',
            '_fields': 'id,joined_at,person.login,person.contacts,group.url,group.name',
            'group.type': 'department'
        }
    department_url = 'yandex_exp_9053'
    url = 'https://staff-api.yandex-team.ru/v3/groupmembership'
    token = wiki_token
    headers = {'Authorization': f'OAuth {token}'}
    persons = []
    async with aiohttp.ClientSession(headers=headers) as session:
        last_id = -1
        while True:
            params = make_params(last_id, department_url)
            async with session.get(url=url, params=params) as response:
                response_data = await response.json()
                if not response_data['result']:
                    break
                last_id = response_data['result'][-1]['id']
                persons += response_data['result']
    print (persons[-1])
    return persons

async def download_staff_details(logins):
    def make_params(min_id, logins):
        return {
            '_sort': 'id',
            '_query': f'id > {min_id} and login in [' + ','.join(['"' + s + '"' for s in logins]) + ']',
            '_fields': 'id,login,name.first,name.last,contacts,official.position.ru,phones,accounts,official.join_at,official.quit_at,official.is_dismissed,yandex'
        }

    url = 'https://staff-api.yandex-team.ru/v3/persons'
    token = wiki_token
    headers = {'Authorization': f'OAuth {token}'}
    persons = []
    async with aiohttp.ClientSession(headers=headers) as session:
        last_id = -1
        while True:
            params = make_params(last_id, logins)
            async with session.get(url=url, params=params) as response:
                response_data = await response.json()
                if not response_data['result']:
                    break
                last_id = response_data['result'][-1]['id']
                persons += response_data['result']
    return persons

def parse_group_record(rec):
    return {
        'id':rec['id'],
        'group_joined_at': rec['joined_at'][:10],
        'login':rec['person']['login'],
        # 'contacts':rec['person']['contacts'],
        'group_url': rec['group']['url'],
        'group_name': rec['group']['name']
        }

cloud_staff = pd.DataFrame([parse_group_record(rec) for rec in asyncio.run(download_persons())])

print (cloud_staff.head())



login_list = np.hstack(cloud_staff.loc[:, ['login']].values)



def parse_person_record(rec):
    if len(rec['accounts']) > 0:
        accounts = {x[0]:x[1] for x in pd.DataFrame([[rec['accounts'][i]['type'],rec['accounts'][i]['value_lower']] for i in range(len(rec['accounts']))]).groupby(0, as_index=False).agg(lambda x: ','.join(x)).values}
    else:
        accounts = dict()
    try:
        quit_at = rec['official']['quit_at']
    except:
        quit_at = '2099-12-31'
    return {
        'id': rec['id'],
        'is_dismissed': str(rec['official']['is_dismissed']),
        'first_name': rec['name']['first']['ru'],
        'last_name': rec['name']['last']['ru'],
        'position': rec['official']['position']['ru'],
        'login': rec['login'],
        'join_at': rec['official']['join_at'],
        'quit_at': quit_at if quit_at!='None' else '2099-12-31',
        'telegram': accounts['telegram'].split(',')[0] if 'telegram' in accounts else '',
        'personal_email': accounts['personal_email'].split(',')[0] if 'personal_email' in accounts else '',
        'telegram_all': accounts['telegram'] if 'telegram' in accounts else '',
        'personal_email_all': accounts['personal_email'] if 'personal_email' in accounts else '',
        'yandex_login' : rec['yandex']['login']
    }

res = []
for j in tqdm(range(cloud_staff.shape[0]//100 + 1)):
    res += [parse_person_record(rec) for rec in  asyncio.run(download_staff_details(login_list[100*j:(100*(j+1))]))]

persons = pd.DataFrame(res)

cols = [
    'id',
    'is_dismissed',
    'login',
    'first_name',
    'last_name',
    'telegram',
    'telegram_all',
    'personal_email',
    'personal_email_all',
    'position',
    'group_url',
    'group_name',
    'join_at',
    'quit_at',
    'group_joined_at',
    'yandex_login'
]



persons2 = pd.merge(cloud_staff,persons, how='left', on='login').rename({'id_y':'id'}, axis=1)[cols]
persons2.to_excel('cloud_staff.xlsx')



cloud_groups = np.hstack(cloud_staff['group_url'].drop_duplicates().values)
print (cloud_groups)



async def get_group_parent(group_id: str):
    def make_params(group_id):
        return {
            '_sort': 'id',
            '_query': f'url == "{group_id}"',
            '_fields': 'url,parent.url,name'
        }

    url = 'https://staff-api.yandex-team.ru/v3/groups'
    token = wiki_token
    headers = {'Authorization': f'OAuth {token}'}
    persons = []
    async with aiohttp.ClientSession(headers=headers) as session:
        params = make_params(group_id)
        try:
            async with session.get(url=url, params=params) as response:
                response_data = await response.json()
                return response_data['result'][0]['parent']['url']
        except:
            return 'no_parent'

async def get_group_name(group_id: str):
    def make_params(group_id):
        return {
            '_sort': 'id',
            '_query': f'url == "{group_id}"',
            '_fields': 'url,parent.url,name'
        }

    url = 'https://staff-api.yandex-team.ru/v3/groups'
    token = wiki_token
    headers = {'Authorization': f'OAuth {token}'}
    persons = []
    async with aiohttp.ClientSession(headers=headers) as session:
        params = make_params(group_id)
        try:
            async with session.get(url=url, params=params) as response:
                response_data = await response.json()
                return response_data['result'][0]['name']
        except:
            return 'no_parent'

print(asyncio.run(get_group_parent('yandex_exp_9053_9308_0409_dep64888')))

def get_group_all_parents(group):
    root_group = 'yandex_exp_9053'
    res = [group]
    parent = asyncio.run(get_group_parent(group))
    if parent == 'no_parent':
        return [group, 'no_parent']
    if (parent != root_group):
        return res + get_group_all_parents(parent)
    else:
        return [group, root_group]


print (get_group_all_parents('yandex_exp_9053_2685')[::-1])

cloud_groups_paths = {}
for group in cloud_groups:
    # print (group)
    if group not in cloud_groups_paths:
        parents = get_group_all_parents(group)
        # print (parents)
        for i, p in enumerate(parents):
            # print (i, p)
            if p not in cloud_groups_paths:
                cloud_groups_paths[p] = parents[i:]

import pprint

pprint.pprint(cloud_groups_paths, width=1)

cloud_groups_names = {g: asyncio.run(get_group_name(g)) for g in cloud_groups}
cloud_groups_names['no_parent'] = 'no_parent'

print (cloud_groups_names)

cloud_groups_paths_names = {g: ' -> '.join([cloud_groups_names[gr] for gr in cloud_groups_paths[g]][::-1]) for g in cloud_groups_paths}




group_pathes = pd.DataFrame([[k, cloud_groups_paths_names[k]] for k in cloud_groups_paths_names.keys()],columns=['group_url', 'group_full_path'])





persons3 = pd.merge(persons2, group_pathes, on='group_url')
# persons3.drop('child_url', axis=1, inplace=True)
persons3['actual_on_date'] = today


# cloud_hierarchy3 = pd.merge(cloud_hierarchy2, cloud_hierarchy_grouped, on='child_url')
# cloud_hierarchy3['actual_on_date'] = today

persons3.to_csv('cloud_staff.csv')
print (persons3[persons3['login'] == 'golubin'])
# cloud_hierarchy3.to_csv('cloud_depts.csv')


yt.wrapper.config.set_proxy("hahn")

persons_tables = ['//home/cloud_analytics/import/staff/cloud_staff/history/'+today, '//home/cloud_analytics/import/staff/cloud_staff/cloud_staff']
persons_schema = [{'name': x, 'type':'string' if x!='id' else 'int64'} for x in persons3.columns]

for table in persons_tables:
    try:
        yt.wrapper.create_table(table, attributes={"schema" : persons_schema})
    except:
        pass
    yt.wrapper.write_table(table, persons3.to_dict(orient='records'))

cols_ = cols + ['group_full_path', 'actual_on_date']
new_cols = {'group_joined_at', 'personal_email_all', 'telegram_all', 'quit_at', 'personal_email', 'yandex_login', 'is_dismissed'}
cols_sql = ['\'\' as ' + x if x in new_cols else x for x in cols_ ]

q = """insert into "<append=%false>//home/cloud_analytics/import/staff/cloud_staff/cloud_staff_history"
SELECT {} FROM "//home/cloud_analytics/import/staff/cloud_staff/history/{}"
UNION ALL
SELECT {} FROM concatYtTablesRange("//home/cloud_analytics/import/staff/cloud_staff/history")
WHERE actual_on_date != '{}'""".format(','.join(cols_), today, ','.join(cols_sql), today)
execute_query_chyt(q)

# cloud_hierarchy_tables = ['//home/cloud_analytics/import/staff/cloud_depts/history/'+today, '//home/cloud_analytics/import/staff/cloud_depts/cloud_depts']
# cloud_hierarchy_schema = [{'name': x, 'type':'string' if x != 'level' else 'int64'} for x in cloud_hierarchy3.columns]



# for table in cloud_hierarchy_tables:
#     try:
#         yt.wrapper.create_table(table, attributes={"schema" : cloud_hierarchy_schema})
#     except:
#         pass
#     yt.wrapper.write_table(table, cloud_hierarchy3.to_dict(orient='records'))

# q = """insert into "<append=%false>//home/cloud_analytics/import/staff/cloud_depts/cloud_depts_history"
# SELECT * FROM concatYtTablesRange("//home/cloud_analytics/import/staff/cloud_depts/history")"""
# execute_query_chyt(q)

print ('SUCCESS')
