import requests
from tokens import wiki_token
import pandas as pd
import yt.wrapper
from datetime import datetime
today = datetime.now().strftime('%Y-%m-%d')

url = 'https://wiki-api.yandex-team.ru/_api/frontend/users/artkaz/Unit-Economy/Sebestoimost-zheleza/.grid'
headers = {
    'Authorization': 'OAuth ' + wiki_token,
    'Content-Type': 'application/json'
}


r = requests.get(url=url, headers=headers)

num_cols = [
    'price'
]

cols = [x['title'] for x in r.json()['data']['structure']['fields']]


df = pd.DataFrame([[x['raw'] for x in x_] for x_ in r.json()['data']['rows']], columns = cols)

for c in num_cols:
    df.loc[:, c] = df.loc[:, c].apply(lambda x: float(x))




schema = [{'name': x, 'type': 'double' if x in set(num_cols) else 'string'} for x in cols]

yt.wrapper.config.set_proxy("hahn")
table1 = '//home/cloud_analytics/tmp/artkaz/pricing_unit_cost'
table2 = '//home/cloud_analytics/cubes/costs/pricing_unit_cost'





for t in [table1, table2]:
    try:
        yt.wrapper.create_table(t, attributes={"schema" : schema})
    except:
        pass

yt.wrapper.write_table(table1, df.to_dict(orient='records'))
yt.wrapper.write_table(table2, df.to_dict(orient='records'))
