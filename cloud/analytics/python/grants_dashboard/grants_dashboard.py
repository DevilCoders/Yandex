import scipy.stats as sps
from sklearn.model_selection import train_test_split
import gc
from dateutil.parser import *
import pandas as pd
import matplotlib.pyplot as plt
import numpy as np
import string
import os
import re
import json

from clan_tools.data_adapters.YTAdapter import YTAdapter
from clan_tools.data_adapters.ClickHouseYTAdapter import ClickHouseYTAdapter
from clan_tools.data_adapters.WikiAdapter import WikiAdapter
import os

yt = YTAdapter(token=os.environ['YT_TOKEN'])
chyt = ClickHouseYTAdapter(token=os.environ['YT_TOKEN'], alias="*ch_public")
wiki = WikiAdapter(token = os.environ['WIKI_TOKEN'])

leads_df = yt.last_table_name('//home/cloud_analytics/dwh/raw/crm/leads')
leads_billing_accounts_df = yt.last_table_name("//home/cloud_analytics/dwh/raw/crm/leads_billing_accounts")
billing_accounts_df = yt.last_table_name("//home/cloud_analytics/dwh/raw/crm/billingaccounts")

df = chyt.execute_query("""
SELECT
    DISTINCT
    type,
    id,
    source_id,
    promocode_proposer,
    ticket,
    created_time,
    start_using_grant_time,
    end_time,
    initial_amount,
    is_activated,
    billing_account_id,
    consumed_amount
FROM (
    SELECT
        DISTINCT 
        'promocode' as type,
        ticket_grants.id as source_id,
        is_activated.id as id,
        visitParamExtractRaw(proposed_meta, 'staffLogin') as promocode_proposer,
        replaceRegexpOne(
        visitParamExtractRaw(upper(proposed_meta), 'REASON') || visitParamExtractRaw(upper(proposed_meta), 'TICKET'), 
        '.*CLOUD(.*)-([\d]*).*', 'CLOUD\\1-\\2') as ticket,
        toDateTime(created_at) as created_time,
        toDateTime(start_time) as start_using_grant_time,
        toFloat32(initial_amount) as initial_amount,
        ifNull(end_time_grant, toDateTime(expiration_time)) as end_time,
        is_activated,
        billing_account_id
    FROM "//home/cloud/billing/exported-billing-tables/monetary_grant_offers_prod" as ticket_grants
    ANY LEFT JOIN (
    SELECT
        DISTINCT
            billing_account_id,
            source_id,
            start_time,
            1 as is_activated,
            id,
            toDateTime(end_time) as end_time_grant
        FROM "//home/cloud/billing/exported-billing-tables/monetary_grants_prod"
    ) as is_activated
    ON ticket_grants.id == is_activated.source_id
    
    UNION ALL
    
    SELECT
        DISTINCT
        'grant' as type,
        source_id,
        id,
        '' as promocode_proposer,
        splitByChar(' ', assumeNotNull(upper(source_id)))[1] as ticket,
        toDateTime(created_at) as created_time,
        toDateTime(start_time) as start_using_grant_time,
        toFloat32(initial_amount) as initial_amount,
        toDateTime(end_time) as end_time,
        1 as is_activated,
        billing_account_id
    FROM "//home/cloud/billing/exported-billing-tables/monetary_grants_prod"
    WHERE lower(source) == 'st'
) as main
LEFT JOIN (
    SELECT
        DISTINCT 
            id,
            if(consumed_amount < 0, 0, consumed_amount) as consumed_amount
    FROM "//home/cloud_analytics/import/billing/grants_spending"
) as consume_info
ON main.id == consume_info.id
WHERE lower(ticket) like 'cloud%'
AND ticket like '%-%'
ORDER BY ticket DESC
""",
to_pandas = True)

lead_source_promocode_df = chyt.execute_query(f"""
SELECT 
    promocode as source_id, 
    any(lead_source) as lead_source_promocode,
    any(lead_source_description) as lead_source_promocode_description
FROM (
SELECT
    a.id as lead_id,
    a.promocode as promocode,
    lead_source,
    lead_source_description,
    billingaccounts_id,
    c.ba_id as ba_id
FROM "{leads_df}" as a
LEFT JOIN (SELECT * FROM "{leads_billing_accounts_df}") as b
ON a.id = b.leads_id
LEFT JOIN (SELECT * FROM "{billing_accounts_df}") as c
ON b.billingaccounts_id = c.id
WHERE promocode is not null
ORDER BY promocode
)
GROUP BY promocode
""",
to_pandas = True)

lead_source_ba_df = chyt.execute_query(f"""
SELECT 
    ba_id as billing_account_id, 
    any(lead_source) as lead_source_ba,
    any(lead_source_description) as lead_source_ba_description
FROM (
SELECT
    a.id as lead_id,
    a.promocode as promocode,
    lead_source,
    lead_source_description,
    billingaccounts_id,
    c.ba_id as ba_id
FROM "{leads_df}" as a
LEFT JOIN (SELECT * FROM "{leads_billing_accounts_df}") as b
ON a.id = b.leads_id
LEFT JOIN (SELECT * FROM "{billing_accounts_df}") as c
ON b.billingaccounts_id = c.id
WHERE promocode is null and ba_id is not null
ORDER BY promocode
)
GROUP BY billing_account_id

""",
to_pandas = True)


df = pd.merge(df, lead_source_promocode_df, on=['source_id'], how='left')

df = pd.merge(df, lead_source_ba_df, on=['billing_account_id'], how='left')

def f(x):
    return x[0] if pd.isnull(x[1]) else x[1]
df['lead_source'] = df[['lead_source_promocode', 'lead_source_ba']].apply(f, axis=1)
df['lead_source_description'] = df[['lead_source_promocode_description', 'lead_source_ba_description']].apply(f, axis=1)


additional_account_info = chyt.execute_query("""
SELECT
    DISTINCT
        billing_account_id,
        first_name,
        last_name,
        phone,
        user_settings_email as email,
        ba_state,
        segment,
        account_name,
        if (client_type == 'company', 'company', 'individual') as client_type,
        sales_name,
        if (first_first_paid_consumption_datetime != '0000-00-00 00:00:00',
            toDate(first_first_paid_consumption_datetime),
            null) as go_to_paid_date,
        if (first_first_trial_consumption_datetime != '0000-00-00 00:00:00',
            toDate(first_first_trial_consumption_datetime),
            null) as go_to_trial_date,
        toDate(first_ba_created_datetime) as ba_created_datetime
FROM "//home/cloud_analytics/cubes/acquisition_cube/cube" as cube
ANY LEFT JOIN (
    SELECT
        DISTINCT 
        billing_account_id,
        'company' as client_type
    FROM (
        SELECT
            DISTINCT
            billing_account_id,
            if (ba_person_type like '%company%', 1, is_corporate_card) as is_company
        FROM "//home/cloud_analytics/cubes/acquisition_cube/cube"
        WHERE is_company == 1
    
        UNION ALL
        SELECT 
            billing_account_id,
            1 as is_company
        FROM "//home/cloud_analytics/import/crm/business_accounts/data"
    )
) as is_company
ON cube.billing_account_id == is_company.billing_account_id
WHERE event == 'ba_created'
OR event == 'cloud_created'
AND billing_account_id != ''

""",
to_pandas = True)


assert len(additional_account_info) == additional_account_info['billing_account_id'].unique().shape[0]


df = pd.merge(df, additional_account_info, how='left', on='billing_account_id')


df['ticket'] = df['ticket'].apply(lambda x: ''.join(x.replace('\\t', "\t").split(string.punctuation)))
df['ticket'] = df['ticket'].apply(lambda x: ''.join(x.replace('\t', "").split(string.punctuation)))


grant_information = wiki.get_data('users/lunin-dv/grants-information-table', to_pandas = True) #lib.get_wiki_table('users/lunin-dv/grants-information-table/')

# grouped_grant_information = grant_information.groupby('ticket')
# rows = []
# for ticket, table in grouped_grant_information:
#     row = {}
#     row['ticket'] = ticket
#     row['grant_company_name'] = ""
#     interest = set(table['grant_company_name']) - set([""])
#     if len(interest) > 0:
#         row['grant_company_name'] = list(interest)[0].strip()
        
#     row['direction'] = f""
#     interest = set(table['direction']) - set([""])
#     if len(interest) > 0:
#         row['direction'] = list(interest)[0]
        
#     row['upsell_experiment_names'] = f""
    
#     interest = " ".join(
#         set([val for x in table['upsell_experiment_names'] for val in x.split(' ')]) - 
#         set([""]))
#     row['upsell_experiment_names'] = interest
#     rows.append(row)

# grant_information = pd.DataFrame(rows)
# grant_information['upsell_experiment_names'].unique()
# lib.replace_wiki_table('users/lunin-dv/grants-information-table/', grant_information)


final_table = pd.merge(df, grant_information, on='ticket', how='left')



final_table['direction'] = final_table['direction'].apply(lambda x: 'unknown' if pd.isnull(x) or x == '' else x)
final_table['grant_company_name'] = final_table[['grant_company_name', 'ticket']].apply(
    lambda x: "" if pd.isnull(x['grant_company_name']) else x['grant_company_name'].replace("'", ""),
    axis=1)

final_table['ticket_query'] = final_table['ticket'].apply(lambda x: x.split('-')[0])




from startrek_client import Startrek
from startrek_client.settings import VERSION_SERVICE



client = Startrek(useragent="robot-clanalytics-yt", 
                  base_url="https://st-api.yandex-team.ru/v2/myself", token=os.environ['TRACKER_TOKEN'])


def ticket_assignee(x):
    try:
        return x.assignee.id
    except Exception:
        return '-'


def ticket_creator(x):
    try:
        return x.createdBy.id
    except Exception:
        return '-'


def ticket_tags(x):
    try:
        if len(x.tags) == 0:
            return '-'
        return (','.join(x.tags)).replace(' ', '')
    except Exception:
        return '-'

def ticket_components(x):
    try:
        components = [component.name for component in x.components]
        if len(components) == 0:
            return '-'
        return (','.join(components)).replace(' ', '')
    except Exception:
        return '-'


def ticket_summary(x):
    try:
        return x.summary
    except Exception:
        return '-'
    
tickets_func = ['ticket_assignee', 'ticket_creator', 'ticket_tags', 'ticket_components', 'ticket_summary']


def all_ticket_info(ticket):
    try:
        ticket_info = client.issues[ticket]
        return {name: globals()[name](ticket_info) for name in tickets_func}
    except Exception:
        print(ticket)
        return {name: '-' for name in tickets_func}


all_ticket_info('CLOUDPS-851')


# Нет доступа к 
# 
# - CLOUDCONTACT
# - CLOUDPROJECTS
# - CLOUDFRONT
# - CLOUDCRM

# In[87]:


ticket_dict = {ticket: all_ticket_info(ticket) for ticket in final_table['ticket'].unique()}


# In[88]:


for name in tickets_func:
    final_table[name] = final_table['ticket'].apply(lambda x: ticket_dict[x][name])


final_table.replace('', '-', inplace=True)


final_table['billing_account_id'].unique().shape




final_table['phone'] = final_table['phone'].apply(lambda x: str(x))
final_table['is_activated'] = final_table['is_activated'].apply(lambda x: float(x))
fillna_schema = {c: 0 if c in ['initial_amount', 'consumed_amount', 'is_activated'] else '' for c in final_table.columns}
final_table = final_table.fillna(fillna_schema)


yt_schema = [{'name': c, 'type': 'double' if c in ['initial_amount', 'consumed_amount', 'is_activated'] else 'string'} for c in final_table.columns]


yt.save_result('//home/cloud_analytics/lunin-dv/grants/offers_grants_information_table', 
               schema = yt_schema,
               df = final_table.fillna(fillna_schema), 
               append=False)

with open('output.json', 'w') as f:
        json.dump({
            "result_table": '//home/cloud_analytics/lunin-dv/grants/offers_grants_information_table',
            "status": 'success'
        }, f)