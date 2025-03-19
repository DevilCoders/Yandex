
#%%
from SwissArmyKnife import SwissArmyKnife, YTClient, ClickHouseClient
import pandas as pd



yt_client = YTClient('Hahn')

vm_stats = yt_client.read_yt_table('/home/cloud_analytics/vm_distribution/2018-12-14')#.iloc[:1000,:]

#%%

ch_settings_df = pd.read_excel('/Users/artkaz/Documents/yacloud/settings/ch_cloud_analytics_connection.xlsx') #sys.argv[1]

ch_settings = {ch_settings_df.iloc[i,:]['field']: ch_settings_df.iloc[i,:]['value'] for i in ch_settings_df.index}


ch_client = ClickHouseClient(user = ch_settings['user'],
                             passw = ch_settings['password'],
                             verify = ch_settings['verify'],
                             host = ch_settings['host']
                             )

vm_stats['sum_quantity'] = vm_stats['sum_quantity'] / 3600 /24

def memory_to_cores(r):
    if int(r) == r:
        return 'c1_m'+str(int(r))
    else:
        return 'c2_m' + str(int(r*2))

vm_stats['memory_to_cores'] = vm_stats['memory_to_cores'].apply(memory_to_cores)





vm_stats_pivot = pd.pivot_table(vm_stats, index= ['cores',
                                                  'memory',
                                                  'cores_fraction',
                                                  'date',
                                                  'folder_id',
                                                  'service_folder',
                                                  'preemptible'
                                                  ],
                                           columns =['memory_to_cores'],
                                           values = ['sum_quantity']).reset_index()

cols = []

for c in vm_stats_pivot.columns:
    if c[0] in {'cores',
                'memory',
                  'cores_fraction',
                  'date',
                  'folder_id',
                  'service_folder',
                  'preemptible'}:
        cols.append( c[0])
    else:
        cols.append(c[1])

vm_stats_pivot.columns = cols

def cores_bucket(r):
    if r['cores_fraction'] == 5:
        return 'burst_' + str(r['cores']).zfill(2)
    else:
        return 'full_' + str(r['cores']).zfill(2)


vm_stats_pivot['cores_bucket'] = vm_stats_pivot[['cores', 'cores_fraction']].apply(cores_bucket, axis=1)
vm_stats['cores_bucket'] = vm_stats[['cores', 'cores_fraction']].apply(cores_bucket, axis=1)

vm_stats_pivot.drop(['cores', 'cores_fraction'], axis=1, inplace=True)
vm_stats.drop(['cores', 'cores_fraction'], axis=1, inplace=True)

vm_stats_pivot['date'] = vm_stats_pivot['date'] + ' 00:00:00'
vm_stats['date'] = vm_stats['date'] + ' 00:00:00'




ch_client.write_df_to_table(vm_stats_pivot, 'cloud_analytics.vm_stats')
ch_client.write_df_to_table(vm_stats, 'cloud_analytics.vm_stats_flat')

