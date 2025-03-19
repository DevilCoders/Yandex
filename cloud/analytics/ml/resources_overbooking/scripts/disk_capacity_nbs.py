# pylint: disable=no-value-for-parameter
import os
os.environ["JAVA_HOME"] = "/usr/local/jdk-11"
os.environ["ARROW_PRE_0_15_IPC_FORMAT"] = "0"
import pip  # needed to use the pip functions
pip.main(['freeze'])

import click
import logging.config
from clan_tools.utils.spark import DEFAULT_SPARK_CONF
from clan_tools.logging.logger import default_log_config
from resources_overbooking.disk_capacity.nbs_forecast import make_forecast_nbs
from resources_overbooking.disk_capacity.config.params_nbs import trained_params

logging.config.dictConfig(default_log_config)
logger = logging.getLogger(__name__)


@click.command()
@click.option('--nbs_prod_used_path', default="//home/cloud_analytics/dwh/ods/nbs/nbs_disk_used_space")
@click.option('--nbs_prod_purch_path', default="//home/cloud_analytics/dwh/ods/nbs/nbs_disk_purchased_space")
@click.option('--nbs_preprod_used_path', default="//home/cloud_analytics/dwh_preprod/ods/nbs/nbs_disk_used_space")
@click.option('--nbs_preprod_purch_path', default="//home/cloud_analytics/dwh_preprod/ods/nbs/nbs_disk_purchased_space")
@click.option('--result_table_path', default="//home/cloud_analytics/resources_overbooking/forecast-nbs")
@click.option('--predict_from', default="None")
@click.option('--predict_to', default="None")
def extract_features(nbs_prod_used_path,
                     nbs_prod_purch_path,
                     nbs_preprod_used_path,
                     nbs_preprod_purch_path,
                     result_table_path,
                     predict_from,
                     predict_to):

    logger.info('Starting source project')
    predict_from = None if (predict_from == "None") else predict_from
    predict_to = None if (predict_to == "None") else predict_to
    make_forecast_nbs(nbs_prod_used_path=nbs_prod_used_path,
                      nbs_prod_purch_path=nbs_prod_purch_path,
                      nbs_preprod_used_path=nbs_preprod_used_path,
                      nbs_preprod_purch_path=nbs_preprod_purch_path,
                      result_table_path=result_table_path,
                      predict_from=predict_from,
                      predict_to=predict_to,
                      params=trained_params,
                      spark_conf=DEFAULT_SPARK_CONF)


if __name__ == '__main__':
    extract_features()
