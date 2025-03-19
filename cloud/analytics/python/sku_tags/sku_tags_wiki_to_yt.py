import requests
#from wiki_token import token
import pandas as pd
from clan_tools.data_adapters.YTAdapter import YTAdapter
from datetime import datetime
today = datetime.now().strftime('%Y-%m-%d')
import json


num_cols = [
    'sku_lazy',
    'cores_number',
    'ram_number',
    'core_fraction_number',
    'is_committed',
    'mk8s_relevant'
]


df = pd.read_csv('sku_tags.csv') #.drop('is_committed', axis=1)
df.loc[:,'subservice_name'].fillna('unknown', inplace=True)
cols = list(df.columns)




for c in num_cols:
    df.loc[:, c] = df.loc[:, c].apply(lambda x: float(x))


yta = YTAdapter()



table1 = '//home/cloud_analytics/tmp/artkaz/sku_tags'
table2 = '//home/cloud_analytics/export/billing/sku_tags/' + today
table3 = '//home/cloud_analytics/export/billing/sku_tags/sku_tags'

table4 = '//home/cloud_analytics/export/billing/sku_tags/sku_tags_preprod'



for t in [table1, table2, table3,]:
    yta.save_result(t, df = df, schema = None, append=False)

with open('output.json', 'w') as f:
        json.dump({
            "status": 'success'
        }, f)

