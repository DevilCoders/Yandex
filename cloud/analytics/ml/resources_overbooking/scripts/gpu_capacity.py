import json
import click
import logging.config
from clan_tools.logging.logger import default_log_config
from clan_tools.utils.timing import timing
from resources_overbooking.gpu.preprocess_data import update_gpu_daily_table

logging.config.dictConfig(default_log_config)
logger = logging.getLogger(__name__)


@timing
@click.command()
@click.option('-r', '--result_table', default='//home/cloud_analytics/resources_overbooking/gpu/daily_table')
@click.option('-p', '--prod_source_path', default='//home/cloud_analytics/resources_overbooking/gpu/import/solomon/prod/1d')
@click.option('-e', '--preprod_source_path', default='//home/cloud_analytics/resources_overbooking/gpu/import/solomon/preprod/1d')
def main(result_table, prod_source_path, preprod_source_path):
    update_gpu_daily_table(result_table, prod_source_path, preprod_source_path)
    with open('output.json', 'w') as f:
        json.dump({"table_path" : result_table}, f)


if __name__ == "__main__":
    main()
