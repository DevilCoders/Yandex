import click
import json
import pandas as pd
import logging.config
from datetime import datetime
from textwrap import dedent
from clan_tools.data_adapters.YQLAdapter import YQLAdapter
from clan_tools.data_adapters.YTAdapter import YTAdapter
from clan_tools.logging.logger import default_log_config
from ml_model_features.utils.crypta_config import segments, web_sites
from yql.client.parameter_value_builder import YqlParameterValueBuilder as ValueBuilder

logging.config.dictConfig(default_log_config)
logger = logging.getLogger(__name__)
# TODO:: add support of cold puids


@click.command()
@click.option('--rebuild', is_flag=True, default=False)
def main(rebuild: bool = False) -> None:
    yql_adapter = YQLAdapter()
    yt_adapter = YTAdapter()
    newline = '\n'

    source_folder_path = '//home/cloud_analytics/ml/ml_model_features/raw/crypta'
    result_folder_path = '//home/cloud_analytics/ml/ml_model_features/by_puid/crypta'

    query = yql_adapter._client.query(dedent(f'''
    PRAGMA yt.Pool = 'cloud_analytics_pool';
    PRAGMA AnsiInForEmptyOrNullableItemsCollections;
    PRAGMA yt.TmpFolder = '//home/cloud_analytics/ml/ml_model_features/tmp';
    Use hahn;
    DECLARE $dates AS List<String>;

    DEFINE ACTION $get_data_for_one_date($date) as
        $base_folder = "{source_folder_path}" || "/" || $date;
        $output = "{result_folder_path}" || "/" || $date;
        $base = (
            SELECT
            `billing_record_msk_date`,
            `puid`,
            MAX(`update_time`) as `update_time`
            FROM $base_folder
            GROUP BY `billing_record_msk_date`, `puid`
        );

        $yuid_count_14_days = (SELECT
            x.`puid` as `puid`,
            x.`billing_record_msk_date` as `date`,
            `yandexuid`,
            COUNT(*) over w as `count`
        FROM $base_folder as x
        JOIN $base as y
        ON x.`update_time` == y.`update_time` AND x.`billing_record_msk_date` == y.`billing_record_msk_date`
        AND x.`puid` == y.`puid`
        WINDOW w AS (
            PARTITION BY x.`puid`, x.`yandexuid`
            ORDER BY x.`billing_record_msk_date`
            ROWS BETWEEN 14 PRECEDING AND CURRENT ROW
        ));

        $good_matches = (SELECT 
            x.`puid` as `puid`,
            x.`date` as `date`,
            `yandexuid`,
            `max_count`,
            `count`
        FROM $yuid_count_14_days as x
        JOIN (SELECT 
            `puid`,
            `date`,
            MAX(`count`) as `max_count`
        FROM $yuid_count_14_days
        GROUP BY `puid`, `date`) as y
        ON x.`puid` == y.`puid` AND x.`date` == y.`date`
        WHERE `count` >= `max_count` * 0.5);

        INSERT INTO $output WITH TRUNCATE
        SELECT
            x.`puid` as `puid`,
            x.`billing_record_msk_date` as `date`,
            {newline.join([f"MAX(`{x}`) as `{x}`," for x in segments])}
            {newline.join([f"MAX(`{'wb_'+ x.replace('.', '_')}`) as `{'wb_'+ x.replace('.', '_')}`," for x in web_sites])}
            MAX(`sd_has_children`) as `sd_has_children`,
            MEDIAN(`sd_proba_age_0_17`) as `sd_proba_age_0_17`,
            MEDIAN(`sd_proba_age_18_24`) as `sd_proba_age_18_24`,
            MEDIAN(`sd_proba_age_25_34`) as `sd_proba_age_25_34`,
            MEDIAN(`sd_proba_age_35_44`) as `sd_proba_age_35_44`,
            MEDIAN(`sd_proba_age_45_54`) as `sd_proba_age_45_54`,
            MEDIAN(`sd_proba_age_55_99`) as `sd_proba_age_55_99`,
            MEDIAN(`sd_proba_gen_f`) as `sd_proba_gen_f`,
            MEDIAN(`sd_proba_gen_m`) as `sd_proba_gen_m`,
            MEDIAN(`sd_proba_in_marriage`) as `sd_proba_in_marriage`,
            MEDIAN(`sd_proba_inc_a`) as `sd_proba_inc_a`,
            MEDIAN(`sd_proba_inc_b1`) as `sd_proba_inc_b1`,
            MEDIAN(`sd_proba_inc_b2`) as `sd_proba_inc_b2`,
            MEDIAN(`sd_proba_inc_c1`) as `sd_proba_inc_c1`,
            MEDIAN(`sd_proba_inc_c2`) as `sd_proba_inc_c2`,
            MEDIAN(`sd_proba_not_in_marriage`) as `sd_proba_not_in_marriage`,
            Math::Round(MEDIAN(`ts_bnpl_decile`)) as `ts_bnpl_decile`,
            Math::Round(MEDIAN(`ts_is_corp_decile`)) as `ts_is_corp_decile`,
            MEDIAN(`ts_proba_bnpl`) as `ts_proba_bnpl`
        FROM $base_folder as x
        JOIN $good_matches as y
        ON x.`puid` == y.`puid` AND x.`billing_record_msk_date` == y.`date`
        AND x.`yandexuid` == y.`yandexuid`
        GROUP BY x.`puid`, x.`billing_record_msk_date`
        ORDER BY `puid`, `date`
    END DEFINE;

    EVALUATE FOR $date IN $dates
        DO $get_data_for_one_date($date)
    '''), syntax_version=1)

    dates_list = yt_adapter.yt.list(source_folder_path)
    if not rebuild:
        dates_list = sorted(dates_list)[-2:]
    else:
        start = datetime.strptime("01-01-2021", "%d-%m-%Y")
        end = datetime.now()
        dates_list = sorted(list(set([str(x)[:7] for x in pd.date_range(start, end)])))
    logger.debug(dates_list)

    parameters = {
        '$dates': ValueBuilder.make_list([ValueBuilder.make_string(dt) for dt in dates_list]),
    }
    query.run(parameters=ValueBuilder.build_json_map(parameters))
    query.get_results()

    for date in dates_list:
        yt_adapter.optimize_chunk_number(f'{result_folder_path}/{date}', retries_num=3)

    with open('output.json', 'w+') as f:
        json.dump({"processed_dates" : dates_list}, f)


if __name__ == "__main__":
    main()
