import json
import click
import logging.config
from clan_tools.logging.logger import default_log_config
from clan_tools.utils.timing import timing
from resources_overbooking.fip4.preprocess_data import update_fip4_daily_table
from resources_overbooking.fip4.collect_data_from_consumption import update_fip4_daily_from_consumption

logging.config.dictConfig(default_log_config)
logger = logging.getLogger(__name__)


@timing
@click.command()
@click.option('--result_table_solomon', default='//home/cloud_analytics/resources_overbooking/fip4/daily_table')
@click.option('--result_table_consumption', default='//home/cloud_analytics/resources_overbooking/fip4/daily_table_consumption')
@click.option('--prod_source_path', default='//home/cloud_analytics/resources_overbooking/fip4/import/solomon/prod/1d')
@click.option('--preprod_source_path', default='//home/cloud_analytics/resources_overbooking/fip4/import/solomon/preprod/1d')
def main(result_table_solomon, result_table_consumption, prod_source_path, preprod_source_path):
    update_fip4_daily_table(result_table_solomon, prod_source_path, preprod_source_path)
    update_fip4_daily_from_consumption(result_table_consumption)
    with open('output.json', 'w') as f:
        json.dump({"result_table_solomon" : result_table_solomon, 'result_table_consumption': result_table_consumption}, f)


if __name__ == "__main__":
    main()
