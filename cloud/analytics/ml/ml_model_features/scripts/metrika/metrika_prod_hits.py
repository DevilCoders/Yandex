import click
import json
import re
import os
import logging.config
from textwrap import dedent
from clan_tools.data_adapters.YQLAdapter import YQLAdapter
from clan_tools.data_adapters.YTAdapter import YTAdapter
from clan_tools.logging.logger import default_log_config
from yql.client.parameter_value_builder import YqlParameterValueBuilder as ValueBuilder

logging.config.dictConfig(default_log_config)
logger = logging.getLogger(__name__)
# TODO:: add support of cold puids

directory = os.getcwd()
logger.debug(directory)


@click.command()
@click.option('--rebuild', is_flag=True, default=False)
def main(rebuild: bool = False) -> None:
    yql_adapter = YQLAdapter()
    yt_adapter = YTAdapter()

    result_folder_path = '//home/cloud_analytics/ml/ml_model_features/by_puid/metrika/hits'
    source_folder_path = '//home/cloud-dwh/data/prod/ods/metrika/hit_log'

    with open('./src/ml_model_features/utils/metrika_key_words.json', 'r') as f:
        data = json.load(f)
    key_words = data['key_words']

    def word_wrapper(word: str) -> str:
        valid_name = re.sub('\.', '_', word)
        return f"SUM(CAST(String::Contains(`start_url`, '{word}') as Int32)) as `{valid_name}`,\n"

    query = yql_adapter.execute_query(dedent(f'''
    PRAGMA yt.Pool = 'cloud_analytics_pool';
    PRAGMA OrderedColumns;
    PRAGMA AnsiInForEmptyOrNullableItemsCollections;
    PRAGMA yt.TmpFolder = '//home/cloud_analytics/ml/ml_model_features/tmp';
    Use hahn;
    DECLARE $dates AS List<String>;

    DEFINE ACTION $get_data_for_one_date($date) as
        $output = "{result_folder_path}" || "/" || $date;
        $pattern = $date || "-%";

        INSERT INTO $output WITH TRUNCATE
        SELECT
            `puid`,
            `date`,
            {"".join([word_wrapper(word) for word in key_words])}
        FROM LIKE(`{source_folder_path}`, $pattern)
        GROUP BY TableName() as `date`, `puid`
    END DEFINE;

    EVALUATE FOR $date IN $dates
        DO $get_data_for_one_date($date)
    '''))

    dates_list = yt_adapter.yt.list(source_folder_path)
    month_list = list(set(map(lambda s: s[:7], dates_list)))

    if not rebuild:
        month_list = sorted(month_list)[-2:]
    logger.debug(month_list)

    parameters = {
        '$dates': ValueBuilder.make_list([ValueBuilder.make_string(dt) for dt in month_list]),
    }
    query.run(parameters=ValueBuilder.build_json_map(parameters))
    query.get_results()

    for date in month_list:
        yt_adapter.optimize_chunk_number(f'{result_folder_path}/{date}', retries_num=3)

    with open('output.json', 'w+') as f:
        json.dump({"processed_dates" : month_list}, f)


if __name__ == "__main__":
    main()
