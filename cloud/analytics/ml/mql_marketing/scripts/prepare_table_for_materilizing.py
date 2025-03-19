import json
import click
import logging.config
from clan_tools.logging.logger import default_log_config
from mql_marketing.materilize_dataset_for_dash import prepare_materilized_table, prepare_mal_table

logging.config.dictConfig(default_log_config)
logger = logging.getLogger(__name__)


@click.command()
@click.option('--path_mal_folder', default='//home/cloud_analytics/ml/mql_marketing/result/mal_list')
@click.option('--path_puids_folder', default='//home/cloud_analytics/ml/mql_marketing/result/by_puids')
@click.option('--path_result_table', default='//home/cloud_analytics/ml/mql_marketing/result/mal_materilized')
@click.option('--path_mal_list', default='//home/cloud_analytics/kulaga/acc_sales_ba_cube')
def update_materilized_table(path_mal_folder: str, path_puids_folder: str, path_result_table: str, path_mal_list: str):

    prepare_mal_table(path_mal_folder, path_puids_folder, path_mal_list)
    prepare_materilized_table(path_mal_folder, path_puids_folder, path_result_table)

    with open('output.json', 'w') as fp:
        json.dump({"Updated table": path_result_table}, fp)

if __name__ == "__main__":
    update_materilized_table()
