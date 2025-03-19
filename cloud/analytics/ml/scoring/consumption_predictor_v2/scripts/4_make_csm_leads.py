# pylint: disable=no-value-for-parameter
import os
print(os.listdir('.'))
os.environ["JAVA_HOME"] = "/usr/local/jdk-11"
os.environ["ARROW_PRE_0_15_IPC_FORMAT"] = "0"
import pip  # needed to use the pip functions
pip.main(['freeze'])

import click
import pickle
import logging.config
from os.path import join as path_join
import spyt
from spyt import spark_session
from datetime import datetime, timedelta
import pyspark.sql.types as T
import pyspark.sql.functions as F
from pyspark.sql.functions import col, lit
from clan_tools.utils.spark import SPARK_CONF_SMALL
from clan_tools.utils.spark import safe_append_spark
from clan_tools.logging.logger import default_log_config
from clan_tools.data_adapters.YTAdapter import YTAdapter
from consumption_predictor_v2.utils.helpers_spark import max_by
from consumption_predictor_v2.train_config.csm_params import PARAMS
from clan_tools.data_adapters.crm.CRMHistoricalDataAdapter import CRMHistoricalDataAdapter

logging.config.dictConfig(default_log_config)
logger = logging.getLogger(__name__)


@click.command()
@click.option('--results_path', default="//home/cloud_analytics/ml/scoring/consumption_predictor_v2/data/prod_results")
@click.option('--features_path', default="//home/cloud_analytics/ml/scoring/consumption_predictor_v2/data/actual_features")
@click.option('--leads_table', default="//home/cloud_analytics/export/crm/update_call_center_leads/update_leads_test")
@click.option('--leads_table_history', default="//home/cloud_analytics/ml/scoring/consumption_predictor_v2/crm/upsell/csm_history_test")
def make_prod_sample(results_path, features_path, leads_table, leads_table_history):
    yt_token = os.environ["SPARK_SECRET"] if ("SPARK_SECRET" in os.environ) else os.environ["YT_TOKEN"]
    yt_adapter = YTAdapter(yt_token)

    contact_info_path = '//home/cloud_analytics/import/crm/leads/contact_info'
    calib_path = '//home/cloud_analytics/ml/scoring/consumption_predictor_v2/model/prod/calibrators_history/csm'
    crm_leads_cube_path = '//home/cloud_analytics/kulaga/leads_cube'

    threshold_th = PARAMS['TARGET_PAID_COND'] // 1000
    pred_colname = f"{threshold_th:.0f}k_pred"
    leads_count = 20
    rep_date = (datetime.now()+timedelta(days=-1)).strftime("%Y-%m-%d")

    calib_name = max(yt_adapter.yt.list(calib_path))
    calibr_ser = yt_adapter.yt.read_file(path_join(calib_path, calib_name)).read()
    calibr = pickle.loads(calibr_ser)

    with spark_session(yt_proxy="hahn", spark_conf_args=SPARK_CONF_SMALL) as spark:
        spyt.info(spark)

        logger.info('Collecting dataset for leads...')
        spdf_info = (
            spark.read.yt(results_path)
            .filter(col("billing_record_msk_date")==rep_date)
            .join(spark.read.yt(features_path), on=["billing_account_id", "billing_record_msk_date"], how="inner")
            .filter(F.coalesce("prev_30d_cons", lit(0)) < PARAMS['TARGET_PAID_COND'])
            .filter(F.coalesce("prev_30d_cons", lit(0)) > PARAMS['MIN_PAID_LAST_30D'])
            .filter(F.coalesce("billing_account_state", lit("active"))=="active")
            .filter(~F.coalesce("billing_account_is_suspended_by_antifraud", lit(False)))
            .filter(~F.coalesce("billing_account_is_isv", lit(False)))
            .filter(~F.coalesce("billing_account_is_var", lit(False)))
            .filter(col("crm_segment").isin(["Mass", "Medium"]))
            .withColumn(pred_colname, col("prev_15d_cons")+col("billing_record_total_rub")+col("next_14d_cons_pred"))
            .select(
                "billing_account_id",
                "billing_record_msk_date",
                "billing_account_usage_status",
                "billing_account_person_type",
                "billing_account_currency",
                "billing_account_state",
                "billing_account_is_fraud",
                "billing_account_is_suspended_by_antifraud",
                "billing_account_is_isv",
                "billing_account_is_var",
                "billing_account_is_crm_account",
                "crm_segment",
                "days_from_created",
                "prev_30d_cons",
                "prev_15d_cons",
                "next_14d_cons_pred",
                pred_colname
            )
        )

        dff = spdf_info["billing_account_id", "billing_record_msk_date", pred_colname].toPandas()
        dff['proba'] = calibr.predict(dff[pred_colname])
        spdf_pred = spark.createDataFrame(dff[["billing_account_id", "billing_record_msk_date", "proba"]])

        spdf_main = spdf_info.join(spdf_pred, on=["billing_account_id", "billing_record_msk_date"], how="left")

        logger.info('Collecting contact and CRM info...')
        spdf_contacts = spark.read.yt(contact_info_path)
        leads_source_1 = (
            CRMHistoricalDataAdapter(yt_adapter, spark)
            .historical_preds()
            .groupby("billing_account_id", col("lead_source_crm").alias("lead_source"))
            .agg(
                max_by('status', 'date_modified').alias('status'),
                F.to_date(F.to_timestamp(F.max("date_entered").astype(T.LongType())/1000000)).alias("lead_date")
            )
            .distinct()
        )
        leads_source_2 = (
            spark.read.yt(crm_leads_cube_path)
            .groupby("billing_account_id", "lead_source")
            .agg(
                max_by('status', 'date_modified').alias('status'),
                F.to_date(F.max("date_entered")).alias("lead_date")
            )
            .distinct()
        )
        crm_leads = (
            leads_source_1
            .union(leads_source_2)
            .distinct()
            .filter(~col("billing_account_id").isNull())
            .filter(~col("billing_account_id").isin(['{{BILLING_ACCOUNT_ID}}', 'billing_id']))
        )
        crm_ba_left_anti = (
            crm_leads.filter(col("lead_source")=='upsell')
            .filter(col('status').isin(['Oppty pending', 'Development', 'Assigned', 'In Process', 'Converted']))
        )

        leads_timestamp = int((datetime.now()+timedelta(hours=3)).timestamp())  # adapting to Moscow timezone (on system is UTC+0)
        spdf_res_leads = (
            spdf_main
            .join(crm_ba_left_anti, on=['billing_account_id'], how="leftanti")
            .join(spdf_contacts, on=['billing_account_id'], how="left")
            .sort(col("proba").desc())
            .filter(col("proba")>0.3)
            .select(
                lit(leads_timestamp).alias('Timestamp'),
                lit(None).astype('string').alias('CRM_Lead_ID'),
                F.concat(lit('["'), "billing_account_id", lit('"]')).alias("Billing_account_id"),
                lit(None).astype('string').alias('Status'),
                F.concat(
                    lit('Confidence that "'),
                    col('billing_account_id'),
                    lit('" is target is '),
                    F.round(col('proba')*100),
                    lit('%.')
                ).alias('Description'),
                lit('admin').alias('Assigned_to'),
                col("first_name").alias('First_name'),
                col("last_name").alias('Last_name'),
                col("phone").alias('Phone_1'),
                lit(None).astype('string').alias('Phone_2'),
                F.concat(lit('["'), col('email'), lit('"]')).alias('Email'),
                lit('upsell').alias('Lead_Source'),
                lit(f'Potential candidate for {threshold_th:.0f}k over 28 days period').alias('Lead_Source_Description'),
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
            .limit(leads_count)
            .coalesce(1)
        )

        safe_append_spark(yt_adapter=yt_adapter, spdf=spdf_res_leads, path=leads_table, mins_to_wait=60)
        safe_append_spark(yt_adapter=yt_adapter, spdf=spdf_res_leads, path=leads_table_history, mins_to_wait=60)

    yt_adapter.optimize_chunk_number(leads_table)
    yt_adapter.optimize_chunk_number(leads_table_history)


if __name__ == '__main__':
    make_prod_sample()
