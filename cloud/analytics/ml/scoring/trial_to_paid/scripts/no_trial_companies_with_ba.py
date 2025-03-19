
import os

os.environ["JAVA_HOME"] = "/usr/local/jdk-11"

from datetime import datetime, timedelta
import logging.config
import os
from datetime import datetime, timedelta
from itertools import chain

from datetime import datetime
import pyspark.sql.dataframe as spd
from pyspark.sql.functions import col, lit
import click
import pyspark.sql.dataframe as spd
import pyspark.sql.functions as F
from clan_tools.logging.logger import default_log_config
from clan_tools.utils.spark import DEFAULT_SPARK_CONF
from pyspark.sql.functions import col
from clan_tools.data_adapters.crm.CRMModelAdapter import  CRMModelAdapter, upsale_to_update_leads
from clan_tools.data_adapters.YTAdapter import YTAdapter

from spyt import spark_session

os.environ["JAVA_HOME"] = "/usr/local/jdk-11"


logging.config.dictConfig(default_log_config)

logger = logging.getLogger(__name__)



def description(days_ago):
    return F.concat(lit('Account "'),
                    col('billing_account_id'),
                    lit(f'" registered {days_ago} days ago.'),
                    lit('Since then it has no consumption.'),
                   ).alias('description')
                   

@click.command()
@click.option('--cloud_cube')
@click.option('--crm_path')
@click.option('--leads_daily_limit')
def leads(cloud_cube: str,  crm_path: str, leads_daily_limit=10):
    with spark_session(yt_proxy="hahn", 
                       spark_conf_args=DEFAULT_SPARK_CONF, 
                       driver_memory='2G') as spark:
        days_ago = 16
        time_from = (datetime.now() - timedelta(days=days_ago)).date()
        yt_adapter = YTAdapter(token=os.environ["SPARK_SECRET"])

        cube = spark.read.yt(cloud_cube)
        companies = (
             cube
                .filter(col('event') == 'ba_created')
                .filter(col('is_isv') == 0)
                .filter(col('br_cost') == 0)
                .filter(col('ba_person_type_actual').contains('company'))
                .filter(col('ba_usage_status_actual') == 'trial')
                .filter(col('segment_actual') == 'Mass')
                .filter(col('is_var') != 'var')
                .filter(F.to_date('event_time') == time_from)
        )

        companies_no_trial = (
             cube
                .filter(col('event') == 'day_use')
                .join(companies.select('billing_account_id'), on='billing_account_id')
                .groupby('billing_account_id')
                .agg(F.sum('br_cost').alias('br_cost'))
                .filter(col('br_cost')==0)
                .select('billing_account_id', 
                         description(days_ago))
                .cache()
        )

        crm_model_adapter = CRMModelAdapter(yt_adapter, spark,
                                            predictions=companies_no_trial,
                                            leads_daily_limit=int(leads_daily_limit),
                                            lead_source=f'NoTrialCompaniesWithBA')
        filtered_leads, _ = crm_model_adapter.save_to_crm()

        (upsale_to_update_leads(filtered_leads, 'trial')
            .write.yt(crm_path, mode='append'))

if __name__ == '__main__':
    leads()


