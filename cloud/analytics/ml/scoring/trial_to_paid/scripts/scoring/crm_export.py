#!/usr/bin/env python
# coding: utf-8

# In[ ]:

import ast
import scipy.stats as sps
from sklearn.model_selection import train_test_split
import gc
from sklearn.linear_model import LinearRegression
from dateutil.parser import *
import pandas as pd
import numpy as np
import os
from collections import Counter
import json
import typing as tp
from sklearn.preprocessing import normalize
import robot_lib as lib
from datetime import datetime
from collections import Counter
from catboost import CatBoostClassifier
import time
import math
from datetime import datetime, timedelta
import yt.wrapper as yt


TABLE_DATE = lib.date_to_string(parse(lib.get_current_date_as_str()) - timedelta(1))
TEST_TABLE_NAME = f"//home/cloud_analytics/scoring_v2/crm_leads/{TABLE_DATE}"


alert_table = lib.execute_query("""
SELECT
    *
FROM "//home/cloud_analytics/scoring_v2/alerts/alert_table"
FORMAT TabSeparatedWithNames
""")


assert alert_table.loc[alert_table['scoring_date'] == TABLE_DATE, 'added_in_crm'].iloc[0] == 0,        "already added in MQL"
# assert alert_table.loc[alert_table['scoring_date'] == TABLE_DATE, 'problems'].iloc[0] == 'OK',        "In this date was unsolved ALERT, if everything is fine, change value"        " in alert table in 'problems' cell "        "state from 'ALERT' to 'OK'"


df = lib.execute_query(f"""
SELECT
    *
FROM "{TEST_TABLE_NAME}"
FORMAT TabSeparatedWithNames
""")
df['description'] = df['description'].apply(lambda x: x.replace('\\n', '\n'))
df['phone'] = df['phone'].astype(str)
df['score_points'] = 5
df['score_type_id'] = 'Lead Score'
df['dwh_score'] = 'inapplicable'
df['description'] = df['description'].apply(lambda x: "model prediction\n" +
                                                                    x + '\n')

df_client_individual = df[df["lead_source_description"] == "Client is Individual"]
df_client_company = df[df["lead_source_description"] != "Client is Individual"]


test, control = train_test_split(df_client_individual, test_size=0.25)

test = pd.concat([test, df_client_company])

MQL_TABLE_NAME = str(datetime.now()).replace(" ", "T")
lib.save_table(MQL_TABLE_NAME, '//home/cloud_analytics/scoring_v2/AB_control_leads', control)
lib.save_table(MQL_TABLE_NAME, '//home/cloud_analytics/scoring_v2/AB_test_leads', test)


lib.save_table(MQL_TABLE_NAME, '//home/cloud_analytics/export/crm/mql', test)

lib.save_table('mql_history', '//home/cloud_analytics/export/crm', test, append=True)

alert_table.loc[alert_table['scoring_date'] == TABLE_DATE, 'added_in_crm'] = 1


lib.save_table('alert_table', '//home/cloud_analytics/scoring_v2/alerts', alert_table)