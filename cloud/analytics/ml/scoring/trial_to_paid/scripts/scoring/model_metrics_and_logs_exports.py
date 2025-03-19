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
import warnings
warnings.simplefilter("ignore")


# %%
CURRENT_DATE = lib.execute_query("""
SELECT
    current_predicting_scoring_date
FROM "//home/cloud_analytics/scoring_v2/helping_folder_for_model/model_scoring_date"
FORMAT TabSeparatedWithNames
""").iloc[0, 0]

# %% [markdown]
# # CRM leads
# 
# ## common_info

# %%
common_information_df = lib.execute_query("""
--sql
SELECT
    billing_account_id,
    user_settings_email as email,
    phone,
    if(first_name == '', 'unknown', first_name) as first_name,
    if(last_name == '', 'unknown', last_name) as last_name,
    'Scoring Leads' as campaign_name,
    account_name as client_name,
    timezone,
    if((ba_person_type like '%company%') OR is_corporate_card = 1, 'Client is Company',
        'Client is Individual') as lead_source_description,
    if (segment IN ('Mass'), 0, 1) as is_managed,
    is_fraud,
    if (first_first_paid_consumption_datetime != '0000-00-00 00:00:00', 1, 0) as is_already_paid
FROM "//home/cloud_analytics/cubes/acquisition_cube/cube" as a
ANY LEFT JOIN(
    SELECT
        passport_uid as puid,
        max(timezone) as timezone
    FROM "//home/cloud_analytics/import/iam/cloud_owners_history"
    GROUP BY passport_uid
) as b
ON a.puid == b.puid
WHERE billing_account_id != ''
AND event == 'ba_created'
AND ba_person_type!='switzerland_nonresident_company'
FORMAT TabSeparatedWithNames
--endsql
""")

# %% [markdown]
# ## scoring_leads_table

# %%
leads_df = lib.execute_query(f"""
SELECT
    *
FROM "//home/cloud_analytics/scoring_v2/helping_folder_for_model/scored_users"
FORMAT TabSeparatedWithNames
""")
leads_df = pd.merge(leads_df, common_information_df, on='billing_account_id', how='inner')


# %%
assert CURRENT_DATE == leads_df['scoring_date'].iloc[0]


# %%
meta_info = lib.MetaInformationClass(interested_columns=[])
meta_info.create_users_id()
res_df = meta_info.get_dataframe_with_grouped_information()


# %%
res_df = res_df[['billing_account_id', 'associated_billings']]
leads_df = pd.merge(leads_df, res_df, on='billing_account_id', how='left')
leads_df = leads_df[~leads_df['associated_billings'].apply(lambda x: not isinstance(x, list) or len(x) > 1)]
leads_df = leads_df.rename(columns={'billing_account_id':'ba_id'})
leads_df.drop(columns = ['associated_billings'], inplace=True)


# %%
def make_description(row):
    paid_proba = round(row['paid_proba'], 3)
    call_proba = round(row['call_proba'], 3)
    text = f"paid_proba: {paid_proba}\n"           f"call_proba: {call_proba}"
    return text


# %%
leads_df['description'] = leads_df.apply(lambda row: make_description(row), axis=1)


# %%
leads_df = leads_df[(leads_df['is_managed'] == 0) &
                    (leads_df['is_fraud'] == 0) &
                    (leads_df['is_already_paid'] == 0)]


# %%
crm_leads = leads_df[(leads_df['paid_prediction'] == 1)]


# %%
crm_leads.drop(columns = ['paid_proba', 'call_proba', 
                          'paid_prediction', 'call_prediction', 'is_managed',
                          'is_fraud', 'is_already_paid'], inplace=True)


# %%
lib.save_table(CURRENT_DATE,
               '//home/cloud_analytics/scoring_v2/all_scoring_leads',
               leads_df)


# %%
lib.save_table(CURRENT_DATE,
               '//home/cloud_analytics/scoring_v2/crm_leads',
               crm_leads)

# %% [markdown]
# # Metrics adding

# %%
def matrics_adding(table_name, folder):
    helping_metrics_df = lib.execute_query(f"""
    SELECT
        *
    FROM "//home/cloud_analytics/scoring_v2/helping_folder_for_model/{table_name}"
    FORMAT TabSeparatedWithNames
    """)
    final_metrics_df = lib.execute_query(f"""
    SELECT
        *
    FROM "//home/cloud_analytics/scoring_v2/{folder}/{table_name}"
    FORMAT TabSeparatedWithNames
    """)
    for col in helping_metrics_df.columns:
        if isinstance(helping_metrics_df[col].iloc[0], list):
            helping_metrics_df[col] = helping_metrics_df[col].astype(str)
            final_metrics_df[col] = final_metrics_df[col].astype(str)
            
    prev_to_change =    final_metrics_df[final_metrics_df['scoring_date'] == helping_metrics_df['scoring_date'].iloc[0]]
    
    if len(prev_to_change) != 0:
        ind = prev_to_change.index[0]
        final_metrics_df.iloc[ind] = helping_metrics_df.iloc[0]
    else:
        final_metrics_df.loc[len(final_metrics_df)] = helping_metrics_df.iloc[0]
    final_metrics_df = final_metrics_df.sort_values(by='scoring_date', ascending=False)
    lib.save_table(table_name, f"//home/cloud_analytics/scoring_v2/{folder}",
                   final_metrics_df)


# %%
names = ['paid', 'call_answer']
table_names = ["_cross_validation_results", '_last_users_validation_results']
for name in names:
    for table_suf in table_names:
        table_name = name + table_suf
        folder = name + "_metrics"
        matrics_adding(table_name, folder)

# %% [markdown]
# # Columns adding

# %%
helping_columns_df = lib.execute_query(f"""
SELECT
    *
FROM "//home/cloud_analytics/scoring_v2/helping_folder_for_model/important_columns"
FORMAT TabSeparatedWithNames
""")
final_columns_df = lib.execute_query(f"""
SELECT
    *
FROM "//home/cloud_analytics/scoring_v2/feature_importance_columns/important_columns"
FORMAT TabSeparatedWithNames
""")
for col in helping_columns_df.columns:
    helping_columns_df[col] = helping_columns_df[col].apply(lambda x: x.replace('\\', ''))
    final_columns_df[col] = final_columns_df[col].apply(lambda x: x.replace('\\', ''))
    
prev_to_change =final_columns_df[final_columns_df['scoring_date'] == helping_columns_df['scoring_date'].iloc[0]]

if len(prev_to_change) != 0:
    ind = prev_to_change.index[0]
    final_columns_df.iloc[ind] = helping_columns_df.iloc[0]
else:
    final_columns_df.loc[len(final_columns_df)] = helping_columns_df.iloc[0]

final_columns_df = final_columns_df.sort_values(by='scoring_date', ascending=False)
lib.save_table('important_columns', "//home/cloud_analytics/scoring_v2/feature_importance_columns",
               final_columns_df)


# %%



