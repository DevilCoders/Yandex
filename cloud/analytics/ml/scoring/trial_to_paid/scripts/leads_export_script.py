# pylint: disable=no-value-for-parameter
import os
print(os.listdir('.'))
os.environ["JAVA_HOME"] = "/usr/local/jdk-11"
os.environ["ARROW_PRE_0_15_IPC_FORMAT"] = "0"
import pip  # needed to use the pip functions
pip.main(['freeze'])

import click
import spyt
from datetime import datetime
from spyt import spark_session
from sklearn.model_selection import train_test_split
from pyspark.sql.functions import col, lit
from clan_tools.utils.spark import SPARK_CONF_MEDIUM
import logging.config
from clan_tools.logging.logger import default_log_config
from clan_tools.data_adapters.YTAdapter import YTAdapter
from clan_tools.data_adapters.crm.CRMHistoricalDataAdapter import CRMHistoricalDataAdapter

logging.config.dictConfig(default_log_config)
logger = logging.getLogger(__name__)


@click.command()
def export_leads(chunk_size: int = 100 * 2 ** 20):
    logger.info("Start collecting Pool for prediction")
    yt_token = os.environ["SPARK_SECRET"] if ("SPARK_SECRET" in os.environ) else os.environ["YT_TOKEN"]
    yt_adapter = YTAdapter(yt_token)

    # prev_model_preds_path = "//home/cloud_analytics/scoring_v2/crm_leads"
    next_model_preds_path = "//home/cloud_analytics/ml/scoring/consumption_predictor_v2/crm/onboarding"
    experiment_path = "//home/cloud_analytics/ml/scoring/consumption_predictor_v2/experiment/onboarding"
    AB_control_path = "//home/cloud_analytics/scoring_v2/AB_control_leads"
    AB_test_path = "//home/cloud_analytics/scoring_v2/AB_test_leads"
    mql_path = "//home/cloud_analytics/export/crm/mql"
    mql_history_table = "//home/cloud_analytics/export/crm/mql_history"
    table = datetime.now().strftime("%Y-%m-%d")
    result_table_name_s = datetime.now().strftime("%Y-%m-%dT%H:%M:%S")
    result_table_name_mks = datetime.now().strftime("%Y-%m-%dT%H:%M:%S.%f")

    with spark_session(yt_proxy="hahn", spark_conf_args=SPARK_CONF_MEDIUM) as spark:
        spyt.info(spark)

        spdf_next = (
            spark.read.yt(f"{next_model_preds_path}/{table}")
            .withColumn("group", lit("New model"))
        )
        # spdf_prev = (
        #     spark.read.yt(f"{prev_model_preds_path}/{table}")
        #     .join(spdf_next, on="ba_id", how="leftanti").withColumn("group", lit("Old model"))
        # )

        filter_crm = (
            CRMHistoricalDataAdapter(yt_adapter, spark)
            .historical_preds()
            .select("billing_account_id", col("lead_source_crm").alias("lead_source"))
            .union(spark.read.yt("//home/cloud_analytics/import/crm/leads/leads_cube").select("billing_account_id", "lead_source"))
            .filter(~col("billing_account_id").isNull())
            .select(col("billing_account_id").alias("ba_id"), "lead_source")
        )

        res_leads = (
            spdf_next
            .join(filter_crm, on="ba_id", how="leftanti")
            .select([
                "ba_id",
                "campaign_name",
                "client_name",
                "description",
                lit("inapplicable").alias("dwh_score"),
                "email",
                "first_name",
                "group",
                "last_name",
                "lead_source_description",
                "phone",
                lit(5).alias("score_points"),
                lit("Lead Score").alias("score_type_id"),
                "scoring_date",
                "timezone"
            ])
        )

        res_leads.coalesce(1).write.yt(f"{experiment_path}/{result_table_name_s}")

        res_comp_leads = res_leads.filter(col("lead_source_description")=="Client is Company")
        res_ind_leads = res_leads.filter(col("lead_source_description")=="Client is Individual")
        test_pre, control = train_test_split(res_ind_leads.toPandas(), test_size=0.25)
        test = res_comp_leads.union(spark.createDataFrame(test_pre)).toPandas()
        test['group'] = 'test'
        control['group'] = 'control'
        colnames = ["ba_id", "scoring_date", "email", "phone", "first_name", "last_name",
                    "campaign_name", "client_name", "timezone", "lead_source_description",
                    "description", "score_points", "score_type_id", "dwh_score"]

        res_test_leads = spark.createDataFrame(test).coalesce(1).select(colnames)
        res_control_leads = spark.createDataFrame(control).coalesce(1).select(colnames)

        res_test_leads.write.yt(f"{AB_test_path}/{result_table_name_mks}")
        res_control_leads.write.yt(f"{AB_control_path}/{result_table_name_mks}")

        res_test_leads.write.yt(f"{mql_path}/{result_table_name_mks}")
        res_test_leads.write.yt(mql_history_table, mode="append")

    yt_adapter.yt.transform(mql_history_table, desired_chunk_size=chunk_size)


if __name__ == '__main__':
    export_leads()
