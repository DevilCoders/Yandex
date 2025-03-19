from pyspark.sql.functions import col, lit
from pyspark.sql.session import SparkSession
from pyspark.sql.dataframe import DataFrame as SparkDataFrame

from clan_tools.data_adapters.YTAdapter import YTAdapter


class CRMHistoricalDataAdapter:

    def __init__(self, yt_adapter: YTAdapter, spark: SparkSession) -> None:
        self._yt_adapter = yt_adapter
        self._spark = spark

    def _get_lead_billing_account_id(self) -> SparkDataFrame:
        lead_ba_map = (
            self._spark.read.yt('//home/cloud-dwh/data/prod/ods/crm/crm_leads_billing_accounts')
            .select(
                col('crm_leads_id').alias('crm_lead_id'),
                col('crm_billing_accounts_id').alias('crm_billing_account_id')
            )
            .filter(col('deleted') == lit(False))
        )

        ba_map = (
            self._spark.read.yt('//home/cloud-dwh/data/prod/ods/crm/billing_accounts')
            .select('crm_billing_account_id', 'billing_account_id')
            .filter(col('deleted') == lit(False))
        )
        lead_ba = (
            lead_ba_map
            .join(ba_map, on='crm_billing_account_id', how='left')
            .select('crm_lead_id', 'billing_account_id')
        )
        return lead_ba

    def _get_lead_user_name(self) -> SparkDataFrame:
        user_name = (
            self._spark.read.yt('//home/cloud-dwh/data/prod/ods/crm/crm_users')
            .select(col('crm_user_id').alias('assigned_user_id'), col('crm_user_name').alias('user_name'))
            .filter(col('deleted') == lit(False))
        )
        return user_name

    def _get_lead_email(self) -> SparkDataFrame:
        bean_rel = (
            self._spark.read.yt('//home/cloud-dwh/data/prod/ods/crm/crm_email_address_bean_relations')
            .select(
                col('bean_id').alias('crm_lead_id'),
                'crm_email_id'
            )
            .filter(col('deleted') == lit(False))
        )
        emails = (
            self._spark.read.yt('//home/cloud-dwh/data/prod/ods/crm/PII/crm_email_addresses')
            .select('crm_email_id', col('email_address').alias('email'))
        )
        lead_email = (
            bean_rel
            .join(emails, on='crm_email_id', how='left')
            .select('crm_lead_id', 'email')
        )
        return lead_email

    def historical_preds(self) -> SparkDataFrame:
        lead_source_list = ['mkt cloud website request', 'upsell', 'var', 'trial', 'isv']

        leads_wo_pii = self._spark.read.yt('//home/cloud-dwh/data/prod/ods/crm/crm_leads')
        leads_pii = self._spark.read.yt('//home/cloud-dwh/data/prod/ods/crm/PII/crm_leads')

        hitorical_crm = (
            leads_wo_pii
            .join(leads_pii, on='crm_lead_id', how='left')
            .filter(col('deleted')==lit(False))
            .filter((col('lead_source').isin(lead_source_list)))
            .join(self._get_lead_billing_account_id(), on='crm_lead_id', how='left')
            .join(self._get_lead_user_name(), on='assigned_user_id', how='left')
            .join(self._get_lead_email(), on='crm_lead_id', how='left')
            .select(
                col('crm_lead_id').alias('lead_id'),
                col('first_name'),
                col('last_name'),
                col('crm_account_name').alias('client_name'),
                col('title'),
                col('phone_mobile').cast("string").alias('phone'),
                col('crm_lead_description').alias('description'),
                col('timezone'),
                col('crm_lead_status').alias('status'),
                col('lead_source_description').alias('lead_source'),
                col('lead_priority'),
                col('date_entered_ts').alias('date_entered'),
                col('date_modified_ts').alias('date_modified'),
                col('billing_account_id'),
                col('user_name'),
                col('email'),
                col('lead_source').alias('lead_source_crm')
            )
        )
        return hitorical_crm.cache()


__all__ = ['CRMHistoricalDataAdapter']
