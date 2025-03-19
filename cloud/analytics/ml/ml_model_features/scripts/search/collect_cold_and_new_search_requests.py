import click
import json
import logging.config
from textwrap import dedent
from clan_tools.data_adapters.YQLAdapter import YQLAdapter
from clan_tools.data_adapters.YTAdapter import YTAdapter
from clan_tools.logging.logger import default_log_config
from ml_model_features.main_operation_search_requests import tokenize_requests

logging.config.dictConfig(default_log_config)
logger = logging.getLogger(__name__)


@click.command()
@click.option('--puids_path', default="//home/cloud_analytics/ml/ml_model_features/raw/new_puids")
@click.option('--result_path', default="//home/cloud_analytics/ml/ml_model_features/by_puid/search_requests")
def main(puids_path: str, result_path: str):
    yql_adapter = YQLAdapter()
    yt_adapter = YTAdapter()

    df = yql_adapter.execute_query(dedent(f'''
    PRAGMA yt.Pool = 'cloud_analytics_pool';
    PRAGMA AnsiInForEmptyOrNullableItemsCollections;
    PRAGMA yt.TmpFolder = '//home/cloud_analytics/ml/ml_model_features/tmp';
    SELECT DISTINCT query_date
    FROM RANGE("{result_path}")
    '''), to_pandas=True)

    dates = list(df.iloc[:, 0].values)
    tokenize_requests(puids_path=puids_path, result_path=result_path, dates_list=dates)
    yt_adapter.optimize_chunk_number(f'{result_path}/{max(dates)[:7]}', retries_num=3)

    with open('output.json', 'w+') as f:
        json.dump({"table_path" : '//home/cloud_analytics/ml/ml_model_features/by_puid/search_requests/'}, f)
        json.dump({"optimized_dates" : max(dates)}, f)


if __name__ == "__main__":
    main()
