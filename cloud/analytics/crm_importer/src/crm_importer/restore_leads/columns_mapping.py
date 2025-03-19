from os import replace
import pyspark.sql.functions as F
from pyspark.sql.functions import col, lit
import pyspark.sql.dataframe as spd


def restore_leads2update_leads(df: spd.DataFrame):
    def add_brackets(col):
        replace_coma = F.regexp_replace(col, r'(,\s?)', '","')
        return F.concat(F.lit('["'), replace_coma, F.lit('"]'))
    cols = [
        F.unix_timestamp().alias('Timestamp'),
        add_brackets(col('billing_account_id')).alias(
            'Billing_account_id'),
        col('set_description').alias('Description'),
        F.coalesce(col('phone_mobile'), lit('-')).alias('Phone_1'),
        add_brackets(col('email')).alias('Email'),
        col('set_lead_source').alias('Lead_Source'),
        col('set_lead_source_description').alias(
            'Lead_Source_Description'),
        col('timezone').alias('Timezone'),
        col('first_name').alias('First_name'),
        col('last_name').alias('Last_name'),
        col('account_name').alias('Account_name'),
        col('set_assigned_to').alias('Assigned_to'),
        add_brackets(col('set_tags')).alias('Tags'),
    ]
    return df.select(cols)


def dm_crm_leads2restore_leads(df: spd.DataFrame):
    cols = [
        col('billing_account_id'),
        col('description').alias('set_description'),
        col('phone_mobile'),
        col('email'),
        col('lead_source').alias('set_lead_source'),
        col('lead_source_description').alias(
            'set_lead_source_description'),
        col('timezone'),
        col('first_name'),
        col('last_name'),
        col('account_name'),
    ]
    return df.select(cols)
