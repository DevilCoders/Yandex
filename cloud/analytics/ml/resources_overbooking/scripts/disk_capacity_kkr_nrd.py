# pylint: disable=no-value-for-parameter
import os
import json
os.environ["JAVA_HOME"] = "/usr/local/jdk-11"
os.environ["ARROW_PRE_0_15_IPC_FORMAT"] = "0"
import pip  # needed to use the pip functions
pip.main(['freeze'])

import click
import logging.config
from clan_tools.utils.spark import DEFAULT_SPARK_CONF
from clan_tools.logging.logger import default_log_config
from resources_overbooking.disk_capacity.kkr_nrd_forecast import make_forecast_kkr_nrd
from resources_overbooking.disk_capacity.config.params_kkr_nrd import trained_params

logging.config.dictConfig(default_log_config)
logger = logging.getLogger(__name__)


@click.command()
@click.option('--kkr_prod_path', default="//home/cloud_analytics/dwh/ods/nbs/kikimr_disk_used_space")
@click.option('--kkr_preprod_path', default="//home/cloud_analytics/dwh_preprod/ods/nbs/kikimr_disk_used_space")
@click.option('--nrd_prod_path', default="//home/cloud_analytics/dwh/ods/nbs/nbs_nrd_used_space")
@click.option('--result_table_path', default="//home/cloud_analytics/resources_overbooking/forecast-kkr-nrd")
@click.option('--predict_from', default="None")
@click.option('--predict_to', default="None")
def extract_features(kkr_prod_path,
                     kkr_preprod_path,
                     nrd_prod_path,
                     result_table_path,
                     predict_from,
                     predict_to):

    logger.info('Starting source project with given params')
    predict_from = None if (predict_from == "None") else predict_from
    predict_to = None if (predict_to == "None") else predict_to
    logger.info(f'Params: \n{json.dumps(trained_params, sort_keys=True, indent=1, ensure_ascii=False)}',)
    make_forecast_kkr_nrd(kkr_prod_path=kkr_prod_path,
                          kkr_preprod_path=kkr_preprod_path,
                          nrd_prod_path=nrd_prod_path,
                          result_table_path=result_table_path,
                          predict_from=predict_from,
                          predict_to=predict_to,
                          params=trained_params,
                          spark_conf=DEFAULT_SPARK_CONF)


if __name__ == '__main__':
    extract_features()
