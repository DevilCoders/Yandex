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
from ml_model_features.utils.crypta_config import segments, web_sites

logging.config.dictConfig(default_log_config)
logger = logging.getLogger(__name__)


@timing
@click.command()
@click.option('--is_prod', is_flag=True, default=False)
@click.option('--rebuild', is_flag=True, default=False)
def copy_crypta_raw(is_prod: bool = False, rebuild: bool = False) -> None:
    logger.info('Starting `copy_crypta_raw` process...')
    yt_adapter = YTAdapter()
    yql_adapter = YQLAdapter()
    newline = '\n'

    source_folder_path = '//statbox/crypta-yandexuid-profiles-log'
    result_folder_path = '//home/cloud_analytics/ml/ml_model_features/raw/crypta'
    test_table = '//home/cloud_analytics/ml/ml_model_features/raw/crypta_test'

    # dates to start and end
    result_table_names = sorted(yt_adapter.yt.list(result_folder_path))
    if len(result_table_names) > 0:
        if rebuild:
            date_from = '2021-01-01'
        else:
            last_table_name = f'{result_folder_path}/{max(result_table_names)}'
            last_loaded_date = yql_adapter.run_query_to_pandas(f'SELECT Max(billing_record_msk_date) FROM `{last_table_name}`').iloc[0, 0]
            date_from = (datetime.strptime(last_loaded_date, '%Y-%m-%d') - timedelta(days=5)).strftime('%Y-%m-%d')
            date_from = max(date_from, '2021-01-01')

        # clear last days after `date_from` in result tables
        related_table_names = [name for name in result_table_names if name >= date_from[:7]]  # find only table names that are affected by `date_from` date
        for table_name in related_table_names:
            yql_adapter.run_query(dedent(f'''
                PRAGMA yt.TmpFolder = '//home/cloud_analytics/ml/ml_model_features/tmp';
                INSERT INTO `{result_folder_path}/{table_name}` WITH TRUNCATE
                    SELECT *
                    FROM `{result_folder_path}/{table_name}`
                    WHERE `billing_record_msk_date` < '{date_from}'
                    ORDER BY `billing_record_msk_date`, `puid`, `update_time`, `yandexuid`;
            '''))
    else:
        date_from = '2021-01-01'
    date_end = max(yt_adapter.yt.list(source_folder_path))

    res_log = {'updated tables': []}
    while date_from <= date_end:
        date_from_dt = datetime.strptime(date_from, '%Y-%m-%d')
        month_end_str = get_last_day_of_month(date_from)
        part_date_end = min(month_end_str, date_end)

        logger.info(f'Loading part {date_from} to {part_date_end} ...')
        insert_table_name = f"{result_folder_path}/{date_from_dt.strftime('%Y-%m')}" if is_prod else test_table
        yql_adapter.run_query(dedent(f'''
        PRAGMA yt.Pool = 'cloud_analytics_pool';
        PRAGMA yt.DefaultCalcMemoryLimit = '512G';
        PRAGMA yt.ParallelOperationsLimit = '256';
        PRAGMA yt.MaxJobCount = '100000';
        PRAGMA yt.TmpFolder = '//home/cloud_analytics/ml/ml_model_features/tmp';
        PRAGMA yt.QueryCacheTtl = '1h';

        $crypta_data =
            SELECT
                TableName() AS `billing_record_msk_date`,
                `yandexuid`,
                `update_time`,
                `gender`,
                `income_5_segments`,
                `user_age_6s`,
                `affinitive_sites`,
                `audience_segments`,
                `lal_internal`,
                `shortterm_interests`,
                `longterm_interests`
            FROM Range(`{source_folder_path}`, `{date_from}`, `{part_date_end}`)
        ;
        INSERT INTO `{insert_table_name}`
            SELECT
                `billing_record_msk_date`,
                crp.`yandexuid` AS `yandexuid`,
                `puid`,
                `update_time`,

                -- soc-dem
                Yson::ConvertToDouble(`gender`['f']) AS `sd_proba_gen_f`,
                Yson::ConvertToDouble(`gender`['m']) AS `sd_proba_gen_m`,
                Yson::ConvertToDouble(`income_5_segments`['A']) AS `sd_proba_inc_a`,
                Yson::ConvertToDouble(`income_5_segments`['B1']) AS `sd_proba_inc_b1`,
                Yson::ConvertToDouble(`income_5_segments`['B2']) AS `sd_proba_inc_b2`,
                Yson::ConvertToDouble(`income_5_segments`['C1']) AS `sd_proba_inc_c1`,
                Yson::ConvertToDouble(`income_5_segments`['C2']) AS `sd_proba_inc_c2`,
                Yson::ConvertToDouble(`user_age_6s`['0_17']) AS `sd_proba_age_0_17`,
                Yson::ConvertToDouble(`user_age_6s`['18_24']) AS `sd_proba_age_18_24`,
                Yson::ConvertToDouble(`user_age_6s`['25_34']) AS `sd_proba_age_25_34`,
                Yson::ConvertToDouble(`user_age_6s`['35_44']) AS `sd_proba_age_35_44`,
                Yson::ConvertToDouble(`user_age_6s`['45_54']) AS `sd_proba_age_45_54`,
                Yson::ConvertToDouble(`user_age_6s`['55_99']) AS `sd_proba_age_55_99`,
                Yson::ConvertToDouble(`lal_internal`['2465']) AS `sd_proba_in_marriage`,
                Yson::ConvertToDouble(`lal_internal`['2466']) AS `sd_proba_not_in_marriage`,
                ListHas(Yson::ConvertToInt64List(`audience_segments`), 19805474) AS `sd_has_children`,

                -- profession
                {newline.join([f'ListHas(Yson::ConvertToInt64List(`audience_segments`), {segments[x]}) AS `{x}`,' for x in segments])}

                -- web sites
                {newline.join([f"Yson::Contains(`affinitive_sites`, '{x}') AS `{'wb_'+ x.replace('.', '_')}`," for x in web_sites])}

                -- trained segments
                CASE
                    WHEN ListHas(Yson::ConvertToInt64List(`audience_segments`), 17044525) THEN 1
                    WHEN ListHas(Yson::ConvertToInt64List(`audience_segments`), 17044528) THEN 2
                    WHEN ListHas(Yson::ConvertToInt64List(`audience_segments`), 17044531) THEN 3
                    WHEN ListHas(Yson::ConvertToInt64List(`audience_segments`), 17044534) THEN 4
                    WHEN ListHas(Yson::ConvertToInt64List(`audience_segments`), 17044537) THEN 5
                    WHEN ListHas(Yson::ConvertToInt64List(`audience_segments`), 17044540) THEN 6
                    WHEN ListHas(Yson::ConvertToInt64List(`audience_segments`), 17044543) THEN 7
                    WHEN ListHas(Yson::ConvertToInt64List(`audience_segments`), 17044546) THEN 8
                    WHEN ListHas(Yson::ConvertToInt64List(`audience_segments`), 17044549) THEN 9
                    WHEN ListHas(Yson::ConvertToInt64List(`audience_segments`), 17044552) THEN 10
                    ELSE NULL
                END AS `ts_is_corp_decile`,
                Coalesce(
                    Yson::ConvertToDouble(`lal_internal`['2515']),
                    Yson::ConvertToDouble(`lal_internal`['2514']),
                    Yson::ConvertToDouble(`lal_internal`['2513']),
                    Yson::ConvertToDouble(`lal_internal`['2512']),
                    Yson::ConvertToDouble(`lal_internal`['2511']),
                    Yson::ConvertToDouble(`lal_internal`['2510']),
                    Yson::ConvertToDouble(`lal_internal`['2509']),
                    Yson::ConvertToDouble(`lal_internal`['2508']),
                    Yson::ConvertToDouble(`lal_internal`['2507']),
                    Yson::ConvertToDouble(`lal_internal`['2516'])
                ) AS `ts_proba_bnpl`,
                CASE
                    WHEN Yson::ConvertToDouble(`lal_internal`['2515']) IS NOT NULL THEN 1
                    WHEN Yson::ConvertToDouble(`lal_internal`['2514']) IS NOT NULL THEN 2
                    WHEN Yson::ConvertToDouble(`lal_internal`['2513']) IS NOT NULL THEN 3
                    WHEN Yson::ConvertToDouble(`lal_internal`['2512']) IS NOT NULL THEN 4
                    WHEN Yson::ConvertToDouble(`lal_internal`['2511']) IS NOT NULL THEN 5
                    WHEN Yson::ConvertToDouble(`lal_internal`['2510']) IS NOT NULL THEN 6
                    WHEN Yson::ConvertToDouble(`lal_internal`['2509']) IS NOT NULL THEN 7
                    WHEN Yson::ConvertToDouble(`lal_internal`['2508']) IS NOT NULL THEN 8
                    WHEN Yson::ConvertToDouble(`lal_internal`['2507']) IS NOT NULL THEN 9
                    WHEN Yson::ConvertToDouble(`lal_internal`['2516']) IS NOT NULL THEN 10
                    ELSE NULL
                END AS `ts_bnpl_decile`

            FROM $crypta_data AS crp
            INNER JOIN `//home/cloud_analytics/ml/ml_model_features/dict/yandexuid_puid` AS yp
                ON crp.`yandexuid` = yp.`yandexuid`
            ORDER BY `billing_record_msk_date`, `puid`, `update_time`, `yandexuid`
            {'' if is_prod else 'LIMIT 100'};'''))
        res_log['updated tables'].append(insert_table_name)
        date_from = (datetime.strptime(part_date_end, '%Y-%m-%d') + timedelta(days=1)).strftime('%Y-%m-%d')
        yt_adapter.optimize_chunk_number(f'{result_folder_path}/{insert_table_name}', retries_num=3)
        if not is_prod:
            break

    with open('output.json', 'w') as fout:
        json.dump(res_log, fout)


if __name__ == "__main__":
    copy_crypta_raw()
