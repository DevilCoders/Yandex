import json
import click
import logging.config
from textwrap import dedent
from datetime import datetime, timedelta
from clan_tools.data_adapters.YQLAdapter import YQLAdapter
from clan_tools.data_adapters.YTAdapter import YTAdapter
from clan_tools.logging.logger import default_log_config
from clan_tools.utils.timing import timing
from ml_model_features.utils.utils import get_last_day_of_month

logging.config.dictConfig(default_log_config)
logger = logging.getLogger(__name__)


@timing
@click.command()
@click.option('--is_prod', is_flag=True, default=False)
@click.option('--rebuild', is_flag=True, default=False)
def copy_metrika_hits_raw(is_prod: bool = False, rebuild: bool = False) -> None:
    logger.info('Starting `copy_metrika_hits_raw` process...')
    yt_adapter = YTAdapter()
    yql_adapter = YQLAdapter()

    source_folder_path = '//logs/hit-log/1d'
    result_folder_path = '//home/cloud_analytics/ml/ml_model_features/raw/metrika/counter_51465824/hits'
    test_table = '//home/cloud_analytics/ml/ml_model_features/raw/metrika/counter_51465824/hits_test'

    # dates to start and end
    result_table_names = yt_adapter.yt.list(result_folder_path)
    if len(result_table_names) > 0 and not rebuild:
        last_table_name = f'{result_folder_path}/{max(result_table_names)}'
        last_loaded_date = yql_adapter.execute_query(f'SELECT Max(billing_record_msk_date) FROM `{last_table_name}`', to_pandas=True).iloc[0, 0]
        date_from = (datetime.strptime(last_loaded_date, '%Y-%m-%d') + timedelta(days=1)).strftime('%Y-%m-%d')
    else:
        last_source_date = min(yt_adapter.yt.list(source_folder_path))
        date_from = (datetime.strptime(last_source_date, '%Y-%m-%d') + timedelta(days=1)).strftime('%Y-%m-%d')
    date_end = max(yt_adapter.yt.list(source_folder_path))

    res_log = {'updated tables': []}
    while date_from <= date_end:
        date_from_dt = datetime.strptime(date_from, '%Y-%m-%d')
        month_end_str = get_last_day_of_month(date_from)
        part_date_end = min(month_end_str, date_end)

        logger.info(f'Loading part {date_from} to {part_date_end} ...')
        insert_table_name = f"{result_folder_path}/{date_from_dt.strftime('%Y-%m')}" if is_prod else test_table
        query = yql_adapter.execute_query(dedent(f'''
        PRAGMA yt.TmpFolder = '//home/cloud_analytics/ml/ml_model_features/tmp';
        PRAGMA yt.QueryCacheTtl = '1h';

        INSERT INTO `{insert_table_name}` {'WITH TRUNCATE' if rebuild else ''}
            SELECT
                DateTime::Format('%Y-%m-%d')(Cast(`EventTime` AS Datetime)) AS `billing_record_msk_date`,
                Cast(`EventTime` AS Datetime) AS `event_time`,
                `PassportUserID` AS `puid`,
                `URL` AS `site_url`,
                `Title` AS `site_title`,
                Coalesce(`DontCountHits`, false) AS `dont_count_hits`
            FROM Range(`{source_folder_path}`, `{date_from}`, `{part_date_end}`) AS t
            WHERE `CounterID` = 51465824 AND `PassportUserID` IS NOT NULL
            ORDER BY `billing_record_msk_date`, `event_time`, `puid`
            {'' if is_prod else 'LIMIT 100'}
        ;'''))
        query.run()
        query.get_results()
        is_yql_query_success = YQLAdapter.is_success(query)
        assert is_yql_query_success, 'Failed'
        res_log['updated tables'].append(insert_table_name)
        date_from = (datetime.strptime(part_date_end, '%Y-%m-%d') + timedelta(days=1)).strftime('%Y-%m-%d')
        yt_adapter.optimize_chunk_number(f'{result_folder_path}/{insert_table_name}', retries_num=3)
        if not is_prod:
            break

    with open('output.json', 'w') as fout:
        json.dump(res_log, fout)


if __name__ == "__main__":
    copy_metrika_hits_raw()
