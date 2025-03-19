# pylint: disable=no-value-for-parameter
import os
os.environ["JAVA_HOME"] = "/usr/local/jdk-11"
os.environ["ARROW_PRE_0_15_IPC_FORMAT"] = "0"
import pip  # needed to use the pip functions
pip.main(['freeze'])

import click
import logging.config
from clan_tools.logging.logger import default_log_config
from resources_overbooking.fip4.fip4_consumption_forecast import make_forecast_fip4_consumtion
from resources_overbooking.fip4.config.params_fip4 import trained_params

logging.config.dictConfig(default_log_config)
logger = logging.getLogger(__name__)


@click.command()
@click.option('--fip4_consumtion_path', default="//home/cloud_analytics/resources_overbooking/fip4/daily_table_consumption")
@click.option('--result_table_path', default="//home/cloud_analytics/resources_overbooking/forecast-fip4")
def main(fip4_consumtion_path, result_table_path):
    logger.info('Starting source project with given params')
    make_forecast_fip4_consumtion(fip4_consumtion_path, result_table_path, trained_params)


if __name__ == '__main__':
    main()
