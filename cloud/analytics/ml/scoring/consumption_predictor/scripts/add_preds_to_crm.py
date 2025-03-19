# pylint: disable=no-value-for-parameter
import os
os.environ["JAVA_HOME"] = "/usr/local/jdk-11"

import click
import logging.config
from datetime import datetime
from spyt import spark_session
from clan_tools.utils.spark import DEFAULT_SPARK_CONF
from clan_tools.data_adapters.YTAdapter import YTAdapter
from clan_tools.data_adapters.crm.CRMModelAdapter import CRMModelAdapter
import pyspark.sql.functions as F
from pyspark.sql.functions import col, lit
from clan_tools.logging.logger import default_log_config
logging.config.dictConfig(default_log_config)

logger = logging.getLogger(__name__)


@click.command('save_to_crm')
@click.option('--predicts_dir')
@click.option('--crm_path')
def save_to_crm(predicts_dir: str, crm_path: str):

    with spark_session(yt_proxy="hahn", spark_conf_args=DEFAULT_SPARK_CONF, driver_memory='2G') as spark:
        yt_adapter = YTAdapter(token=os.environ["SPARK_SECRET"])

        def _descr():
            return F.concat(lit('Confidence that "'),
                            col('billing_account_id'),
                            lit('" is target is '),
                            F.round(col('conf_is_target')*100), lit('%.'))

        preds = spark.read\
            .yt(yt_adapter.last_table_name(predicts_dir))\
            .filter(col('conf_is_target') > 0.01) \
            .select(col('billing_account_id'),
                    col('conf_is_target').alias('sort_column'),
                    _descr().alias('description'))

        crm_model_adapter = CRMModelAdapter(yt_adapter, spark,
                                            predictions=preds,
                                            leads_daily_limit=6,
                                            lead_source='Potential candidate for 50k over 28 days period')

        filtered_preds, _ = crm_model_adapter.save_to_crm()
        table_name = datetime.now().strftime('%Y-%m-%dT%H:%M:%S')
        filtered_preds.write.yt(f'{crm_path}/{table_name}')

    # with open('output.json', 'w') as f:
    #         json.dump(asdict(experiment), f)


if __name__ == '__main__':
    save_to_crm()
