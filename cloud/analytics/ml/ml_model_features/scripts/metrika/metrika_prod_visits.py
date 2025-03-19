import click
import json
import logging.config
from textwrap import dedent
from clan_tools.data_adapters.YQLAdapter import YQLAdapter
from clan_tools.data_adapters.YTAdapter import YTAdapter
from clan_tools.logging.logger import default_log_config
from yql.client.parameter_value_builder import YqlParameterValueBuilder as ValueBuilder

logging.config.dictConfig(default_log_config)
logger = logging.getLogger(__name__)
# TODO:: add support of cold puids


@click.command()
@click.option('--rebuild', is_flag=True, default=False)
def main(rebuild: bool = False) -> None:
    yql_adapter = YQLAdapter()
    yt_adapter = YTAdapter()

    source_folder_path = '//home/cloud_analytics/ml/ml_model_features/raw/metrika/counter_51465824/visits'
    result_folder_path = '//home/cloud_analytics/ml/ml_model_features/by_puid/metrika/visits'

    with open('./src/ml_model_features/yql/metrika_visits.sql', 'r') as f:
        query = yql_adapter.execute_query(dedent(f.read()))

    dates_list = yt_adapter.yt.list(source_folder_path)
    if not rebuild:
        dates_list = sorted(dates_list)[-2:]
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
