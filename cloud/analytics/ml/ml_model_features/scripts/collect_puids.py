import json
import logging.config
import click
from textwrap import dedent
from clan_tools import utils
from clan_tools.data_adapters.YQLAdapter import YQLAdapter
from clan_tools.logging.logger import default_log_config

logging.config.dictConfig(default_log_config)
logger = logging.getLogger(__name__)


@click.command()
@click.option('--cold_puids_path', default="//home/cloud_analytics/ml/ml_model_features/by_puid/cold_puids")
@click.option('--new_puids_path', default="//home/cloud_analytics/ml/ml_model_features/raw/new_puids")
@click.option('--all_puids_path', default="//home/cloud_analytics/ml/ml_model_features/raw/all_puids")
@click.option('--known_puids_path', default="//home/cloud-dwh/data/prod/ods/iam/passport_users")
def main(cold_puids_path: str, new_puids_path: str, all_puids_path: str, known_puids_path: str):
    yql_adapter = YQLAdapter()

    query = yql_adapter.execute_query(dedent(f'''
    PRAGMA yt.Pool = 'cloud_analytics_pool';
    PRAGMA AnsiInForEmptyOrNullableItemsCollections;

    $cold_puids = (
    SELECT
        distinct passport_uid
        FROM (select passport_uid FROM `{cold_puids_path}`) as a
        LEFT ONLY JOIN
        (select passport_uid FROM `{all_puids_path}`) as b
        ON a.passport_uid = b.passport_uid
    );

    $new_puids = (
    SELECT
        distinct passport_uid
        FROM (select passport_uid FROM `{known_puids_path}`) as a
        LEFT ONLY JOIN
        (select passport_uid FROM `{all_puids_path}`) as b
        ON a.passport_uid = b.passport_uid
    );

    INSERT INTO `{new_puids_path}` WITH TRUNCATE
    SELECT * FROM $cold_puids
    UNION ALL
    SELECT * FROM $new_puids;

    INSERT INTO `{all_puids_path}`
    SELECT * FROM $cold_puids
    UNION ALL
    SELECT * FROM $new_puids
    '''))
    YQLAdapter.attach_files(utils.__file__, 'yql', query)
    query.run()
    query.get_results()
    is_success = YQLAdapter.is_success(query)
    if is_success:
        with open('output.json', 'w+') as f:
            json.dump({"table_path" : all_puids_path}, f)


if __name__ == "__main__":
    main()
