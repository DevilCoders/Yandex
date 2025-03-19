import click
import json
import logging.config
from datetime import timedelta, datetime
from textwrap import dedent
from ml_model_features.main_operation_search_requests import tokenize_requests
from clan_tools.data_adapters.YQLAdapter import YQLAdapter
from clan_tools.logging.logger import default_log_config

logging.config.dictConfig(default_log_config)
logger = logging.getLogger(__name__)


@click.command()
@click.option('--puids_path', default="//home/cloud_analytics/ml/ml_model_features/raw/all_puids")
@click.option('--result_path', default="//home/cloud_analytics/ml/ml_model_features/by_puid/search_requests")
def main(puids_path: str, result_path: str):
    yql_adapter = YQLAdapter()

    df = yql_adapter.execute_query(dedent(f'''
    PRAGMA yt.Pool = 'cloud_analytics_pool';
    PRAGMA AnsiInForEmptyOrNullableItemsCollections;

    $now = CurrentUtcTimestamp();
    $format = DateTime::Format("%Y-%m-%d");

    SELECT Max(query_date) as start_date, CAST($format( CAST($now AS Date) - Interval("P2D")) AS Date) as end_date
    FROM RANGE("{result_path}")
    '''), to_pandas=True)
    start_date = datetime.strptime(df['start_date'][0], '%Y-%m-%d').date()
    end_date = df['end_date'][0]

    delta = end_date - start_date
    dates = list(set([str(start_date + timedelta(days=i)) for i in range(1, delta.days + 1)]))
    logger.debug(f"Recent Dates:\n {dates}")
    tokenize_requests(puids_path=puids_path, result_path=result_path, dates_list=dates)

    with open('output.json', 'w+') as f:
        json.dump({"table_path" : '//home/cloud_analytics/ml/ml_model_features/by_puid/search_requests/'}, f)


if __name__ == "__main__":
    main()
