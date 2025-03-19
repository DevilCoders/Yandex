import logging.config
import os

import click
from pyspark import java_gateway
import pyspark.sql.dataframe as spd
import pyspark.sql.functions as F
import pyspark.sql.types as T
from clan_tools.logging.logger import default_log_config
from clan_tools.utils.spark import DEFAULT_SPARK_CONF
from pyspark.sql.functions import array_distinct, col, flatten, lit, udf
from spyt import spark_session
from datetime import datetime, timedelta
from clan_tools.data_adapters.YTAdapter import YTAdapter
from clan_tools.data_adapters.crm.CRMHistoricalDataAdapter import CRMHistoricalDataAdapter
import os
from itertools import chain



os.environ["JAVA_HOME"] = "/usr/local/jdk-11"


logging.config.dictConfig(default_log_config)

logger = logging.getLogger(__name__)





@click.command()
@click.option('--cold_calls_scores')
@click.option('--onboarding_leads')
@click.option('--result_path')
def cold_calls(cold_calls_scores: str, onboarding_leads: str, result_path:str):
    with spark_session(yt_proxy="hahn", spark_conf_args=DEFAULT_SPARK_CONF, driver_memory='2G') as spark:

        data_with_preds = spark.read.yt(cold_calls_scores)
        onboarding_leads = spark.read.yt(onboarding_leads)
        yt_adapter = YTAdapter(token=os.environ['SPARK_SECRET'])


        inn_preds = (
            data_with_preds
            .groupby('inn')
            .agg(F.max('score').alias('score'))
            
        )

        cold_calls = (
            onboarding_leads
            .filter(col('match')==0)  
            .filter(col('min_level')==-1)  
            .join(inn_preds, on='inn')
        )
        historical_data_adapter = CRMHistoricalDataAdapter(yt_adapter, spark)
        historical_preds = historical_data_adapter.historical_preds()

        isv_cold_call_leads = (
            historical_preds
            .filter(col('lead_source_crm')=='isv')
            .filter(col('lead_source')=='cold-calling')
            .cache()
        )


        filtered_cold_calls = (
            cold_calls
            .join(isv_cold_call_leads, how='left_anti', 
              on=isv_cold_call_leads.client_name==cold_calls.spark_name)
        )


        (
            filtered_cold_calls
            .withColumn('sort_col', -1*col('score'))
            .sort('sort_col')
            .limit(2000)
            .write
            .sorted_by('sort_col')
            .mode("overwrite")
            .yt(result_path)
        )
        


if __name__ == '__main__':
    cold_calls()
