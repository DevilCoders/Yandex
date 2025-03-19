#!/usr/bin/env python
# coding: utf-8

# In[218]:


import pandas as pd
from fbprophet import Prophet
from clickhouse_connection.password import ch_password
from io import StringIO
from scipy import stats
import numpy as np
import yt.wrapper

# In[219]:


from tqdm import tqdm


# In[220]:


import requests

def execute_query(query,
                  user= 'artkaz',
                  password = ch_password,
                  verify = 'clickhouse_connection/allCAs.pem',
                  host = 'https://vla-2z4ktcci90kq2bu2.db.yandex.net:8443',
                  timeout=600):
    #logger.info("Executing query: %s", query)
    s = requests.Session()
    resp = s.post(host, data=query, timeout=timeout, auth=(user, password), verify=verify)
    if resp.status_code != 200:
        print (resp.status_code, resp.headers, resp.content)
    resp.raise_for_status()
    rows = resp.content.strip()#.split('\n')
    #logger.info("Time spent: %s seconds, rows returned: %s", resp.elapsed.total_seconds(), len(rows))
    return rows
print (execute_query('SELECT now()'))

def q_to_df(q, cols, date_col = ''):
        s = execute_query(q).decode('UTF-8')
        df = pd.read_csv(StringIO(s), sep='\t', names = cols)
        if (date_col == ''):
            return df
        else:
            df.loc[:, date_col] = df.loc[:, date_col].apply(lambda d: mdates.date2num(datetime(*[int(item) for item in d.split('-')])))
            return df


q = """
SELECT
    toDate(event_time) as ds,
    subservice_name,
    sum(real_consumption_vat) as y
FROM
   cloud_analytics.acquisition_cube
WHERE 1=1
    AND ds >= '2019-10-01'
    AND ds < toDate(now())
    AND service_name = 'cloud_ai'
    AND subservice_name in ('speech', 'mt', 'vision')
GROUP BY ds, subservice_name
ORDER BY ds, subservice_name
"""

data = q_to_df(q, ['ds', 'subservice', 'y'])



subservices = data['subservice'].drop_duplicates()



def get_forecast(data_, days=365, service='unknown'):
    data = data_.copy()
    data.loc[:,'y'] = data.loc[:,'y'].apply(lambda x: x if x<1500000 else np.nan)
    print (data.ds.max())
    m = Prophet()
    m.fit(data)
    future = m.make_future_dataframe(periods = days)
    result = m.predict(future)[['ds', 'yhat', 'yhat_lower', 'yhat_upper']]
    result.loc[:,'ds'] = result.loc[:,'ds'].apply(lambda t: str(t)[:10])
    result['service'] = service
    return pd.melt(pd.merge(data, result, on='ds', how='right'), id_vars = ['ds', 'service'], value_vars = ['y', 'yhat', 'yhat_lower', 'yhat_upper'])


get_forecast (data.loc[(data['subservice'] == 'mt'),['ds', 'y']])

result = []
for subservice in tqdm(['speech','mt','vision']):
    print (subservice)
    result.append(get_forecast(data.loc[(data['subservice'] == subservice),['ds', 'y']], service = subservice + '_actual'))

    result.append(get_forecast(data.loc[(data['subservice'] == subservice),['ds', 'y']].iloc[:-30,:], service = subservice + '_test'))

    data_actual = data.loc[(data['subservice'] == subservice),['ds', 'y']].iloc[-30:,:]
    data_actual['service'] = subservice + '_test'
    data_actual['variable'] = 'y'
    data_actual.rename({'y':'value'}, axis=1, inplace=True)

    result.append(data_actual)

df_res = result[0]

for df_ in result[1:]:
    df_res = df_res.append(df_)



create_table_statement = """

CREATE TABLE cloud_analytics.ml_revenue_forecast(
  ds Date,
  service String,
  variable String,
  value Float64
) ENGINE = MergeTree() ORDER BY(ds, service, variable)

"""

def write_df_to_table(df, table, overwrite=True):
    s = StringIO()
    df.to_csv(s, index=False)
    df_csv = s.getvalue()[s.getvalue().find('\n') + 1:]
    df_cols = s.getvalue()[:s.getvalue().find('\n')]
    df_insert_query = 'INSERT INTO ' +\
                        table +\
                         ' (' +\
                         df_cols +\
                         ') FORMAT CSV ' +\
                         df_csv
    if overwrite:
        try:
            execute_query('DROP TABLE ' + table)
        except:
            pass
        execute_query(create_table_statement)

    execute_query(df_insert_query)

write_df_to_table(df_res[:], 'cloud_analytics.ml_revenue_forecast')


yt.wrapper.config.set_proxy("hahn")

schema = [
            {'name': 'ds', 'type': 'string'},
            {'name': 'service', 'type': 'string'},
            {'name': 'variable', 'type': 'string'},
            {'name': 'value', 'type': 'double'}
        ]
t = '//home/cloud_analytics/ml_metrics/ml_revenue_forecast/ml_revenue_forecast'
try:
    yt.wrapper.create_table(t, attributes={"schema" : schema})
except:
    print('table already exists:', t)


yt.wrapper.write_table(t, df_res.to_dict(orient='records'))





