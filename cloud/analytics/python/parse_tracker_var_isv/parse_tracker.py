
# coding: utf-8

# In[1]:


import requests
import pandas as pd
from tokens import token

import sys
stdout = sys.stdout
reload(sys)
sys.setdefaultencoding('utf8')
sys.stdout = stdout
from SwissArmyKnife import ClickHouseClient

# In[2]:


url = 'https://st-api.yandex-team.ru/v2/issues?filter=queue:CLOUDVAR'



r = requests.get(url,headers={
                'Authorization': 'OAuth %s' % token
            } )


ch_settings_df = pd.read_csv('/home/artkaz/ch_cloud_analytics_connection.csv') #sys.argv[1]

ch_settings = {ch_settings_df.iloc[i,:]['field']: ch_settings_df.iloc[i,:]['value'] for i in ch_settings_df.index}


ch_client = ClickHouseClient(user = ch_settings['user'],
                             passw = ch_settings['password'],
                             verify = ch_settings['verify'],
                             host = ch_settings['host']
                             )

def return_queue_list(q):
    url = 'https://st-api.yandex-team.ru/v2/issues?filter=queue:%s&page=1&perPage=1000' %q
    r = requests.get(url,headers={
                    'Authorization': 'OAuth %s' % token
                } )
    df = pd.DataFrame.from_dict([{'id':  j['key'],
                             'created_at': j['createdAt'],
                             'assignee': j['assignee']['id'] if 'assignee' in j else 'unknown',
                             'status': j['status']['key']
                             } for j in r.json()], orient='columns', )


    return df


var_stats = return_queue_list('CLOUDVAR')
isv_stats = return_queue_list('CLOUDISV')
var_isv_stats = var_stats.append(isv_stats)

var_isv_stats['created_at'] = var_isv_stats['created_at'].apply(lambda d: d[:19].replace('T', ' '))


print (var_isv_stats.shape)
print (var_isv_stats.columns)

hosts = [ch_settings['host'], ch_settings['host2']]
for h in hosts:
    print (h)
    ch_client = ClickHouseClient(user = ch_settings['user'],
                             passw = ch_settings['password'],
                             verify = ch_settings['verify'],
                             host = h
                             )
    ch_client.write_df_to_table(var_isv_stats, 'cloud_analytics.var_isv_requests')

print( 'SUCCESS')
