import requests
from wiki_token import token

url = 'https://wiki-api.yandex-team.ru/_api/frontend/users/artkaz/eventscosts/.grid'
headers = {
    'Authorization': token,
    'Content-Type': 'application/json'
}

r = requests.get(url=url, headers=headers)

cols = [
    'event_id',
    'event_name', 
    'event_type', 
    'event_q', 
    'event_date', 
    'event_costs', 
    'distribute_by', 
    'mailing_id', 
    'distr_by_segment_only',
    'event_segment',
    'ba_list',
    'comment'
    ]

import pandas as pd 
df = pd.DataFrame([[x['raw'] for x in x_] for x_ in r.json()['data']['rows']], columns = cols)
df.loc[:, 'event_costs'] = df.loc[:, 'event_costs'].apply(lambda x: float(x) if not pd.isnull(x) else 0.0)
df.loc[:, 'distr_by_segment_only'] = df.loc[:, 'distr_by_segment_only'].apply(lambda x: int(x) if not pd.isnull(x) else 0.0)

df.loc[:, 'mailing_id'] = df.loc[:, 'mailing_id'].apply(lambda x: x.split(';') if not pd.isnull(x) else [])
df.loc[:, 'ba_list'] = df.loc[:, 'ba_list'].apply(lambda x: x.split(';') if not pd.isnull(x) else [])

print (df)


import yt.wrapper
yt.wrapper.config.set_proxy("hahn")

dtypes = {
    'event_id':'string',
    'event_name' : 'string', 
    'event_type' : 'string', 
    'event_q' : 'string', 
    'event_date' : 'string', 
    'event_costs' : 'double', 
    'distribute_by' : 'string', 
    'mailing_id' : 'any', 
    'distr_by_segment_only' : 'int32',
    'event_segment' : 'string',
    'ba_list' : 'any',
    'comment' : 'string'
}

schema = [{'name': x, 'type': dtypes[x]} for x in df.columns]


#print (schema)
#print(df.head().to_dict(orient = 'records'))

table = '//home/cloud_analytics/events/events_costs'

try:
    yt.wrapper.create_table(table, attributes={"schema" : schema})
except:
    yt.wrapper.remove(table)
    yt.wrapper.create_table(table, attributes={"schema" : schema})

yt.wrapper.write_table(table, df.to_dict(orient='records'))


print ('SUCCESS')