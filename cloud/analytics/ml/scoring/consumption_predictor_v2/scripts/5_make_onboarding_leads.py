# pylint: disable=no-value-for-parameter
import os
print(os.listdir('.'))
os.environ["JAVA_HOME"] = "/usr/local/jdk-11"
os.environ["ARROW_PRE_0_15_IPC_FORMAT"] = "0"
import pip  # needed to use the pip functions
pip.main(['freeze'])

import click
from spyt import spark_session
import logging.config
from datetime import datetime, timedelta
import spyt
import pyspark.sql.functions as F
from pyspark.sql.functions import col, lit
from pyspark.sql.column import Column as SparkColumn
from clan_tools.utils.spark import SPARK_CONF_LARGE
from clan_tools.utils.spark import safe_append_spark
from clan_tools.logging.logger import default_log_config
from clan_tools.data_adapters.YTAdapter import YTAdapter
from consumption_predictor_v2.train_config.onb_params import PARAMS
from clan_tools.data_adapters.crm.CRMHistoricalDataAdapter import CRMHistoricalDataAdapter


logging.config.dictConfig(default_log_config)
logger = logging.getLogger(__name__)


@click.command()
@click.option('--results_path', default="//home/cloud_analytics/ml/scoring/consumption_predictor_v2/data/prod_results")
@click.option('--features_path', default="//home/cloud_analytics/ml/scoring/consumption_predictor_v2/data/actual_features")
@click.option('--leads_table', default="//home/cloud_analytics/export/crm/update_call_center_leads/update_leads_test")
@click.option('--leads_table_history', default="//home/cloud_analytics/ml/scoring/consumption_predictor_v2/crm/onboarding/onb_history_test")
def make_prod_sample(results_path, features_path, leads_table, leads_table_history):
    logger.info("Start collecting Pool for prediction")
    yt_token = os.environ["SPARK_SECRET"] if ("SPARK_SECRET" in os.environ) else os.environ["YT_TOKEN"]
    yt_adapter = YTAdapter(yt_token)

    dm_crm_tags = '//home/cloud-dwh/data/prod/cdm/dm_ba_crm_tags'
    contact_info_path = '//home/cloud_analytics/import/crm/leads/contact_info'
    history_window = 15  # calculate for previous period to exclude skipping leads

    with spark_session(yt_proxy="hahn", spark_conf_args=SPARK_CONF_LARGE) as spark:
        spyt.info(spark)

        spdf_crm_tags = (
            spark.read.yt(dm_crm_tags)
            .select(
                'billing_account_id',
                'account_owner_current',
                'usage_status_current',
                'segment_current',
                'state_current',
                'person_type_current',
                'is_suspended_by_antifraud_current',
                'is_var_current',
                'is_isv_current'
            )
            .distinct()
            .cache()
        )

        employee_and_practicum = (
            spark.read.yt('//home/cloud-dwh/data/prod/ods/billing/monetary_grants')
            .filter(col('source') == 'employee')
            .filter(col('end_time') > datetime.now())
            .select('billing_account_id')
            .union(spark.read.yt('//home/cloud_analytics/yc-operations/datasets/practicum/base')
            .select('yc_ba_id').alias('billing_account_id'))
            .filter(col('billing_account_id').isNotNull())
            .distinct()
            .cache()
        )

        paid_company_inn = (
            spark.read.yt('//home/cloud_analytics/ml/scoring/consumption_predictor_v2/data/paid_inn')
            .select('billing_account_id')
            .filter(col('billing_account_id').isNotNull())
            .distinct()
            .cache()
        )

        spdf_contacts = spark.read.yt(contact_info_path).cache()

        def _trial_lead_source_desc() -> SparkColumn:
            """Creates lead source description:
             - if `person_type_current` corresponds to individual no matter resident or no -> `Client is Individual`
             - if `person_type_current` corresponds to company -> `Client is Company`
             - if `person_type_current` corresponds to nonresident company -> `TrialCompaniesNonresidents`
            """
            is_nonres_company = (col('person_type_current').rlike('company')) & (col('person_type_current') != 'company')
            company_or_individual = F.when(col('person_type_current').rlike('individual'), lit('individual')).otherwise(col('person_type_current'))
            common_lead_source_desc = F.concat(lit("Client is "), F.initcap(company_or_individual))
            return F.when(is_nonres_company, lit("TrialCompaniesNonresidents")).otherwise(common_lead_source_desc)

        def _sales_lead_source_desc() -> SparkColumn:
            """Sales lead source description
            """
            is_nonres_company = (col('person_type_current').rlike('company')) & (col('person_type_current') != 'company')
            return F.when(is_nonres_company, lit('payment not confirmed_nonres')).otherwise(lit('payment not confirmed'))

        for i in range(1, history_window+1):
            rep_date = (datetime.now()+timedelta(days=-i)).strftime("%Y-%m-%d")
            logger.info(f'Leads as of {rep_date}')

            spdf_forecast = (
                spark.read.yt(results_path)
                .filter(col("billing_record_msk_date")==rep_date)
                .join(spark.read.yt(features_path), on=["billing_account_id", "billing_record_msk_date"], how="left")
                .join(spdf_crm_tags, on="billing_account_id", how="left")
                .select(
                    'billing_account_id',
                    'billing_record_msk_date',
                    'next_14d_cons_pred',
                    (col('billing_record_total_rub')+col('prev_14d_cons')).alias('last_15d_paid'),
                    (col('billing_record_credit_rub')+col('prev_14d_grants')).alias('last_15d_grants'),
                    'days_from_created',
                    'account_owner_current',
                    'usage_status_current',
                    'segment_current',
                    'state_current',
                    'person_type_current',
                    'is_suspended_by_antifraud_current',
                    'is_var_current',
                    'is_isv_current'
                )
            )

            spdf_forecast_ind = (
                spdf_forecast
                .filter(col('days_from_created')==PARAMS['DAYS_INDIVIDUAL'])
                .filter(col('person_type_current').rlike('individual'))
                .filter(F.coalesce('last_15d_paid', lit(0))==0)
                .filter(F.coalesce('last_15d_grants', lit(0))<0)
                .filter(col('next_14d_cons_pred')>0)
                .filter(F.coalesce('state_current', lit('active'))=='active')
                .filter(~F.coalesce('is_suspended_by_antifraud_current', lit(False)))
                .filter(~F.coalesce('is_isv_current', lit(False)))
                .filter(~F.coalesce('is_var_current', lit(False)))
                .filter(col('account_owner_current')=='No Account Owner')
                .sort(col('next_14d_cons_pred').desc())
                .select(
                    '*',
                    lit('trial').alias('Lead_Source'),
                    _trial_lead_source_desc().alias('Lead_Source_Description')
                )
                .distinct()
                .limit(45)
            )

            spdf_forecast_comp = (
                spdf_forecast
                .filter(col('days_from_created')==PARAMS['DAYS_COMPANIES'])
                .filter(col('person_type_current').rlike('company'))
                .filter(F.coalesce('last_15d_paid', lit(0))==0)
                .sort(col('next_14d_cons_pred').desc())
                .filter(F.coalesce('state_current', lit('active')) == 'active')
                .filter(~F.coalesce('is_suspended_by_antifraud_current', lit(False)))
                .filter(~F.coalesce('is_isv_current', lit(False)))
                .filter(~F.coalesce('is_var_current', lit(False)))
                .filter(col('account_owner_current')=='No Account Owner')
                .select(
                    '*',
                    lit('trial').alias('Lead_Source'),
                    _trial_lead_source_desc().alias('Lead_Source_Description')
                )
            )

            # switched off that part of lead-flow CLOUDANA-1966
            spdf_forecast_sales_comp = (
                spdf_forecast
                .filter(col('days_from_created')==PARAMS['DAYS_COMPANIES'])
                .filter(col('person_type_current').rlike('company'))
                .filter(F.coalesce('last_15d_paid', lit(0))==0)
                .sort(col('next_14d_cons_pred').desc())
                .filter(F.coalesce('state_current', lit('active')) == 'payment_not_confirmed')
                .filter(~F.coalesce('is_suspended_by_antifraud_current', lit(False)))
                .filter(~F.coalesce('is_isv_current', lit(False)))
                .filter(~F.coalesce('is_var_current', lit(False)))
                .filter(col('account_owner_current')=='No Account Owner')
                .select(
                    '*',
                    lit('sales').alias('Lead_Source'),
                    _sales_lead_source_desc().alias('Lead_Source_Description')
                )
                .filter(col('Lead_Source_Description') != 'payment not confirmed_nonres')
                .join(paid_company_inn, on='billing_account_id', how='leftanti')
            )

            logger.info(f'Individual: {spdf_forecast_ind.count()}')
            logger.info(f'Companies: {spdf_forecast_comp.count()}')
            logger.info(f'Sales: {spdf_forecast_sales_comp.count()}')

            crm_bas = CRMHistoricalDataAdapter(yt_adapter, spark).historical_preds()[['billing_account_id']].distinct()
            crm_ongoing = (
                spark.read.yt(leads_table)
                .select(F.regexp_replace('Billing_account_id', r'[\[\]"]', '').alias('billing_account_id'))
                .distinct()
            )

            crm_exclude = crm_bas.union(crm_ongoing).distinct()

            leads_timestamp = int((datetime.now()+timedelta(hours=3)).timestamp())  # adapting to Moscow timezone (on system is UTC+0)
            df_res_leads = (
                spdf_forecast_ind
                .union(spdf_forecast_comp)
                .union(spdf_forecast_sales_comp)
                .join(crm_exclude, on='billing_account_id', how='leftanti')
                .join(employee_and_practicum, on='billing_account_id', how='leftanti')
                .join(spdf_contacts, on='billing_account_id', how='left')
                .select(
                    lit(leads_timestamp).alias('Timestamp'),
                    lit(None).astype('string').alias('CRM_Lead_ID'),
                    F.concat(lit('["'), "billing_account_id", lit('"]')).alias("Billing_account_id"),
                    lit(None).astype('string').alias('Status'),
                    F.concat(lit('Used grants on '),
                             F.round(-col('last_15d_grants')+1-1, 2),  #
                             lit(' rub on day '),
                             col('days_from_created'),
                             lit(' after BA creation')).alias('Description'),
                    lit('admin').alias('Assigned_to'),
                    col('first_name').alias('First_name'),
                    col('last_name').alias('Last_name'),
                    col('phone').alias('Phone_1'),
                    lit(None).astype('string').alias('Phone_2'),
                    F.concat(lit('["'), col('email'), lit('"]')).alias('Email'),
                    'Lead_Source',
                    'Lead_Source_Description',
                    lit(None).astype('string').alias('Callback_date'),
                    lit(None).astype('string').alias('Last_communication_date'),
                    lit(None).astype('string').alias('Promocode'),
                    lit(None).astype('string').alias('Promocode_sum'),
                    lit(None).astype('string').alias('Notes'),
                    lit(None).astype('string').alias('Dimensions'),
                    lit(None).astype('string').alias('Tags'),
                    lit('').alias('Timezone'),
                    col("display_name").alias('Account_name')
                )
                .distinct()
                .coalesce(1)
            )
            logger.info(f'Total leads (after removing current CRM leads): {df_res_leads.count()}')

            safe_append_spark(yt_adapter=yt_adapter, spdf=df_res_leads, path=leads_table, mins_to_wait=60)
            safe_append_spark(yt_adapter=yt_adapter, spdf=df_res_leads, path=leads_table_history, mins_to_wait=60)

    yt_adapter.optimize_chunk_number(leads_table)
    yt_adapter.optimize_chunk_number(leads_table_history)


if __name__ == '__main__':
    make_prod_sample()
