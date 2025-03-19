#!/usr/bin/env python
# coding: utf-8

# In[40]:


import sys
path = '/home/artkaz/arc/arcadia/cloud/analytics/python/lib/clan_tools/src'
if not (path in sys.path):
    sys.path.append(path)
from clan_tools.data.holidays_in_russia import holidays_in_russia_df


# In[41]:


holidays = holidays_in_russia_df[holidays_in_russia_df['holiday'] == 1].rename({'date':'ds'}, axis=1)


# In[42]:


holidays['holiday'] = 'holiday'


# In[43]:


holidays


# In[44]:


import pandas as pd
from fbprophet import Prophet
from ch_connection import ch_password
from io import StringIO
from scipy import stats
import numpy as np
import yt.wrapper

from clan_tools.data_adapters.ClickHouseAdapter import ClickHouseAdapter

from tqdm import tqdm

ch = ClickHouseAdapter( user= 'artkaz',
                        password = ch_password
)

print(ch.execute_query('SELECT now() as now',to_pandas=True))


# In[45]:


q = """
SELECT
    toDate(event_time) as ds,
    subservice_name as subservice,
    sum(real_consumption_vat) as y
FROM
   cloud_analytics.acquisition_cube
WHERE 1=1
    AND ds >= '2019-10-01'
    AND ds < '2020-05-31'
    AND service_name = 'cloud_ai'
    AND subservice_name in ('speech', 'mt', 'vision')
GROUP BY ds, subservice_name
ORDER BY ds, subservice_name
"""


# In[46]:


data = ch.execute_query(q, to_pandas=True)
subservices = data['subservice'].drop_duplicates()


# In[47]:


data


# In[48]:


subservices


# In[49]:


def get_forecast(data_, days=365, service='unknown'):
    data = data_.copy()
    data.loc[:,'y'] = data.loc[:,'y'].apply(lambda x: x if x<1500000 else np.nan)
    print (data.ds.max())
    m = Prophet(holidays=holidays)
    m.fit(data)
    future = m.make_future_dataframe(periods = days)
    result = m.predict(future)[['ds', 'yhat', 'yhat_lower', 'yhat_upper']]
#     print(result.iloc[200:250,:])
    result.loc[:,'ds'] = result.loc[:,'ds'].apply(lambda t: str(t)[:10])
    result['service'] = service
    result = result.loc[result['ds'] > data['ds'].max(),:]
#     print (result)
#     print (pd.merge(data, result, on='ds', how='right'))
    return pd.melt(pd.merge(data, result, on='ds', how='right'), id_vars = ['ds', 'service'], value_vars = ['y', 'yhat', 'yhat_lower', 'yhat_upper'])

get_forecast (data.loc[(data['subservice'] == 'mt'),['ds', 'y']], 365, 'mt').head(-10)


# In[50]:


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


# In[51]:


create_table_statement = """

CREATE TABLE cloud_analytics.ml_revenue_forecast(
  ds Date,
  service String,
  variable String,
  value Float64
) ENGINE = MergeTree() ORDER BY(ds, service, variable)

"""


# In[52]:


def write_df_to_table(df, table, overwrite=True):
    s = StringIO()
    df.to_csv(s, index=False)
    df_csv = s.getvalue()[s.getvalue().find('\n') + 1:]
    df_cols = s.getvalue()[:s.getvalue().find('\n')]
    df_insert_query = 'INSERT INTO ' +                        table +                         ' (' +                         df_cols +                         ') FORMAT CSV ' +                         df_csv
    if overwrite:
        try:
            ch.execute_query('DROP TABLE ' + table)
        except:
            pass
        ch.execute_query(create_table_statement)

    ch.execute_query(df_insert_query)

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

print ('SUCCESS')


# In[ ]:





# In[ ]:




