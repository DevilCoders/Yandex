import json
import click
import logging.config
from textwrap import dedent
from datetime import datetime, timedelta
from clan_tools.data_adapters.YQLAdapter import YQLAdapter
from clan_tools.data_adapters.YTAdapter import YTAdapter
from clan_tools.logging.logger import default_log_config
from ml_model_features.utils.utils import get_last_day_of_month

logging.config.dictConfig(default_log_config)
logger = logging.getLogger(__name__)


@click.command()
@click.option('--is_prod', is_flag=True, default=False)
@click.option('--rebuild', is_flag=True, default=False)
def copy_crypta_all_raw(is_prod: bool = False, rebuild: bool = False) -> None:
    logger.info('Starting `copy_crypta_raw` process...')
    yt_adapter = YTAdapter()
    yql_adapter = YQLAdapter()

    source_folder_path = '//statbox/crypta-yandexuid-profiles-log'
    result_folder_path = '//home/cloud_analytics/ml/ml_model_features/raw/crypta_all'
    test_table = '//home/cloud_analytics/ml/ml_model_features/raw/crypta_all_test'

    # dates to start and end
    result_table_names = sorted(yt_adapter.yt.list(result_folder_path))
    if len(result_table_names) > 0:
        if rebuild:
            date_from = '2022-01-01'
        else:
            last_table_name = f'{result_folder_path}/{max(result_table_names)}'
            last_loaded_date = yql_adapter.run_query_to_pandas(f'SELECT Max(billing_record_msk_date) FROM `{last_table_name}`').iloc[0, 0]
            date_from = (datetime.strptime(last_loaded_date, '%Y-%m-%d') - timedelta(days=5)).strftime('%Y-%m-%d')
            date_from = max(date_from, '2021-01-01')

        # clear last days after `date_from` in result tables
        related_table_names = [name for name in result_table_names if name >= date_from[:7]]  # find only table names that are affected by `date_from` date
        for table_name in related_table_names:
            query = yql_adapter.run_query(dedent(f'''
                PRAGMA yt.TmpFolder = '//home/cloud_analytics/ml/ml_model_features/tmp';
                INSERT INTO `{result_folder_path}/{table_name}` WITH TRUNCATE
                    SELECT *
                    FROM `{result_folder_path}/{table_name}`
                    WHERE `billing_record_msk_date` < '{date_from}'
                    ORDER BY `billing_record_msk_date`, `puid`, `update_time`, `yandexuid`;
            '''))
            query.run()
            query.get_results()
            is_yql_query_success = YQLAdapter.is_success(query)
            assert is_yql_query_success, 'Failed YQL-query'
    else:
        date_from = '2022-01-01'
    date_end = max(yt_adapter.yt.list(source_folder_path))

    res_log = {'updated tables': []}
    while date_from <= date_end:
        date_from_dt = datetime.strptime(date_from, '%Y-%m-%d')
        month_end_str = get_last_day_of_month(date_from)
        part_date_end = min(month_end_str, date_end)

        logger.info(f'Loading part {date_from} to {part_date_end} ...')
        insert_table_name = f"{result_folder_path}/{date_from_dt.strftime('%Y-%m')}" if is_prod else test_table
        query = yql_adapter.run_query(dedent(f'''
        PRAGMA yt.Pool = 'cloud_analytics_pool';
        PRAGMA yt.DefaultCalcMemoryLimit = '512G';
        PRAGMA yt.ParallelOperationsLimit = '256';
        PRAGMA yt.TmpFolder = '//home/cloud_analytics/ml/ml_model_features/tmp';
        PRAGMA yt.MaxJobCount = '100000';

        $crypta_data =
            SELECT
                TableName() AS `billing_record_msk_date`,
                t.*
            FROM Range(`{source_folder_path}`, `{date_from}`, `{part_date_end}`) AS t
        ;
        INSERT INTO `{insert_table_name}`
            SELECT
                `billing_record_msk_date`,
                crp.`yandexuid` AS `yandexuid`,
                `puid`,
                `update_time`,
                `exact_socdem`,
                `gender`,
                `user_age_6s`,
                `age_segments`,
                `income_5_segments`,
                `income_segments`,
                `yandex_loyalty`,
                `longterm_interests`,
                `shortterm_interests`,
                `interests_composite`,
                `audience_segments`,
                `probabilistic_segments`,
                `marketing_segments`,
                `heuristic_segments`,
                `heuristic_internal`,
                `heuristic_private`,
                `heuristic_common`,
                `lal_common`,
                `lal_internal`,
                `lal_private`,
                `top_common_sites`,
                `top_common_site_ids`,
                `affinitive_sites`,
                `affinitive_site_ids`
            FROM $crypta_data AS crp
            INNER JOIN `//home/cloud_analytics/ml/ml_model_features/dict/yandexuid_puid` AS yp
                ON crp.`yandexuid` = yp.`yandexuid`
            ORDER BY `billing_record_msk_date`, `puid`, `update_time`, `yandexuid`
            {'' if is_prod else 'LIMIT 100'}
        ;'''))
        query.run()
        query.get_results()
        is_yql_query_success = YQLAdapter.is_success(query)
        assert is_yql_query_success, 'Failed YQL-query'
        res_log['updated tables'].append(insert_table_name)
        date_from = (datetime.strptime(part_date_end, '%Y-%m-%d') + timedelta(days=1)).strftime('%Y-%m-%d')
        yt_adapter.optimize_chunk_number(f'{result_folder_path}/{insert_table_name}', retries_num=3)
        if not is_prod:
            break

    with open('output.json', 'w') as fout:
        json.dump(res_log, fout)


if __name__ == "__main__":
    copy_crypta_all_raw()
