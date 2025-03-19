import scipy.stats as sps
from sklearn.model_selection import train_test_split
import gc
from dateutil.parser import *
import pandas as pd
import numpy as np
import os
from datetime import datetime
import random
import warnings
from datetime import timedelta
import os
import calendar
import sys
import os.path
import time
import typing as tp
import json
import math
import requests
import robot_lib as lib
import ast

import time
time.sleep(20)
date = lib.get_current_date_as_str()


df = lib.execute_query(f"""
SELECT
    toInt64(NOW()) as Timestamp,
    associated_billings as Billing_account_id,
    'New' as Status,
    'admin' as Assigned_to,
    phone as Phone_1,
    email as Email,
    timezone as Timezone,
    account_name as Account_name,
    ifNull(first_name, '') as First_name,
    ifNull(last_name, '') as Last_name,
    'upsell' as Lead_Source,
    'Churn Prediction' as Lead_Source_Description,
    'person type: ' || person_type ||
    '; summary paid consumption per last 30 days: ' || toString(paid_consumption_per_last_month)  
    as Description
FROM "//home/cloud_analytics/churn_prediction/churn_prediction_test_group/{date}"
FORMAT TabSeparatedWithNames
""")


df['Billing_account_id'] = df['Billing_account_id'].apply(lambda y: 
        ['dn' + x.split('dn')[1] for x in y.split(';') if len(x.split('dn')) > 1])
df["Email"] = df["Email"].apply(lambda x: [x])
df['Phone_1'] = df['Phone_1'].astype(str)

arr_cols = ['Email', 'Billing_account_id']
for col in arr_cols:
    df[col] = df[col].astype(str)
    df[col] = df[col].apply(lambda x: x.replace("'", '"') if not pd.isnull(x) else np.nan)


lib.save_table('update_leads', "//home/cloud_analytics/export/crm/update_call_center_leads",
               df, append=True)