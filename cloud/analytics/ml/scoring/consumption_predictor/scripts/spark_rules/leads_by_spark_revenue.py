import logging.config
import os
import click
import pyspark.sql.dataframe as spd
import pyspark.sql.functions as F
import pyspark.sql.types as T
from clan_tools.logging.logger import default_log_config
from clan_tools.utils.spark import SPARK_CONF_GRAPHS
from pyspark.sql.functions import col, lit
from spyt import spark_session
import pandas as pd
from pyspark.sql.functions import pandas_udf
from pyspark.sql.session import SparkSession
from clan_tools.data_adapters.YTAdapter import YTAdapter
from clan_tools.data_adapters.crm.CRMModelAdapter import  CRMModelAdapter, upsale_to_update_leads

os.environ["JAVA_HOME"] = "/usr/local/jdk-11"


logging.config.dictConfig(default_log_config)

logger = logging.getLogger(__name__)


def description():
    url = 'https://datalens.yandex-team.ru/cfoaznpqmzbg6-organizationsidgraphdash?level='
    return F.concat(lit('Account "'),
                    col('billing_account_id'),
                    lit('" has consumption '),
                    F.round(col('real_consumption_180'), 2),
                    lit(f'rub over last 180 days in total аnd belongs to company "'),
                    col('spark_name'),
                    lit('" (ИНН '), col('inn'),
                    lit(f') with last known revenue '),
                    F.round(col('company_size_revenue'), 2),
                    lit(f'mln rub over year and workers range '),
                    col('workers_range'),
                    lit('. Lead of level '),
                    col('level'), 
                    lit(f'. {url}'),
                    col('level'),
                    lit('&node_ids=billing_account_id::'),
                    col('billing_account_id'),
                    lit('&node_ids=inn::'),
                    col('inn')
                   ).alias('description')


@click.command('leads_by_revenue')
@click.option('--csm_leads')
@click.option('--crm_path')
@click.option('--leads_daily_limit')
def leads_by_revenue(csm_leads: str,  crm_path: str, leads_daily_limit=10):
    with spark_session(yt_proxy="hahn", spark_conf_args=SPARK_CONF_GRAPHS, driver_memory='2G') as spark:
        yt_adapter = YTAdapter(token=os.environ["SPARK_SECRET"])

        csm_leads = spark.read.yt(csm_leads)
        
        csm_leads_descriptions = (
            csm_leads
                .select('billing_account_id', 
                        col('sort_col').alias('sort_column'), 
                        description())
        )

        crm_model_adapter = CRMModelAdapter(yt_adapter, spark,
                                            predictions=csm_leads_descriptions,
                                            leads_daily_limit=int(leads_daily_limit),
                                            lead_source=f'Big SMB Companies')


        filtered_leads, _ = crm_model_adapter.save_to_crm()

        (upsale_to_update_leads(filtered_leads)
            .write.yt(crm_path, mode='append'))

       


if __name__ == '__main__':
    leads_by_revenue()
