# pylint: disable=no-value-for-parameter
import logging.config
import os
from datetime import datetime

import click
# import numpy as np
# import pandas as pd
from clan_tools.data_adapters.YTAdapter import YTAdapter
from clan_tools.logging.logger import default_log_config
from clan_tools.utils.spark import DEFAULT_SPARK_CONF
from pyspark.sql.functions import lit
from spyt import spark_session

os.environ["JAVA_HOME"] = "/usr/local/jdk-11"


logging.config.dictConfig(default_log_config)
logger = logging.getLogger(__name__)


@click.command('save_to_crm')
@click.option('--crm_path')
def save_to_crm(crm_path: str):

    with spark_session(yt_proxy="hahn", spark_conf_args=DEFAULT_SPARK_CONF, driver_memory='2G') as spark:

        # main_dataset = 'CLOUDANA-1418'
        # main_model = 'CLOUDANA-1353'

        hypothesis_dataset = 'CLOUDANA-1418'
        hypothesis_model = 'CLOUDANA-1353'

        # leads_root = '//home/cloud_analytics/smb/targets_from_mass'
        # main_leads_dir = f'{leads_root}/{main_dataset}/{main_model}/crm_leads'
        hypothesis_leads_dir = '//home/cloud_analytics/ml/scoring/consumption_predictor_v2/crm/upsell'
        # hypothesis_leads_dir = f'{leads_root}/{hypothesis_dataset}/{hypothesis_model}/crm_leads'

        yt_adapter = YTAdapter(token=os.environ["SPARK_SECRET"])

        # last_main_leads = yt_adapter.last_table_name(main_leads_dir)
        last_hypothesis_leads = yt_adapter.last_table_name(hypothesis_leads_dir)

        # model_leads = (spark.read.yt(last_main_leads).withColumn('tracker_id', lit(f'{main_dataset},{main_model}')))
        hypothesis_leads = (spark.read.yt(last_hypothesis_leads)
                            .withColumn('tracker_id', lit(f'{hypothesis_dataset},{hypothesis_model}')))

        # final_model_leads = model_leads.join(hypothesis_leads, on='billing_account_id', how='leftanti')

        # model_cnt = final_model_leads.count()
        hypot_cnt = hypothesis_leads.count()
        # index_model = pd.Series(range(model_cnt)).sample(min(model_cnt, 3)).tolist()
        # index_hypothesis = pd.Series(range(hypot_cnt)).sample(min(6-min(model_cnt, 3), hypot_cnt)).tolist()
        # index_model = np.random.randint(0, 2, 4).astype(bool)
        # index_hypothesis = ~index_model

        # result_leads = pd.concat([final_model_leads.toPandas().loc[index_model],
        #                           hypothesis_leads.toPandas().loc[index_hypothesis]]).sample(frac=1.)

        if last_hypothesis_leads.split('/')[-1].split('T')[0] == datetime.now().strftime('%Y-%m-%d') and hypot_cnt>0:
            result_leads = hypothesis_leads.toPandas()
            result_leads_exp = hypothesis_leads.withColumn("group", lit("New model")).toPandas()
            table_name = datetime.now().strftime('%Y-%m-%dT%H:%M:%S')
            spark.createDataFrame(result_leads).coalesce(1).write.yt(f'{crm_path}/{table_name}')
            spark.createDataFrame(result_leads_exp).coalesce(1).write.yt(
                f'//home/cloud_analytics/ml/scoring/consumption_predictor_v2/experiment/csm/{table_name}'
            )

        # result_leads_exp = pd.concat([
        #     final_model_leads.withColumn("group", lit("Old model")).toPandas().loc[index_model],
        #     hypothesis_leads.withColumn("group", lit("New model")).toPandas().loc[index_hypothesis]]
        # )

        # table_name = datetime.now().strftime('%Y-%m-%dT%H:%M:%S')

        # spark.createDataFrame(result_leads).coalesce(1).write.yt(f'{crm_path}/{table_name}')
        # spark.createDataFrame(result_leads_exp).coalesce(1).write.yt(f'//home/cloud_analytics/ml/scoring/consumption_predictor_v2/experiment/csm/{table_name}')


if __name__ == '__main__':
    save_to_crm()
