import logging.config
from textwrap import dedent
from clan_tools import utils
from clan_tools.data_adapters.YQLAdapter import YQLAdapter
from clan_tools.data_adapters.YTAdapter import YTAdapter
from clan_tools.logging.logger import default_log_config
from datetime import datetime, timedelta
from clan_tools.utils.timing import timing

logging.config.dictConfig(default_log_config)
logger = logging.getLogger(__name__)


def leave_only_last_table(ytadapter, folder):
    all_tables = sorted([f'{folder}/{table}' for table in ytadapter.list(folder)])
    for table in all_tables[:-1]:
        ytadapter.remove(table)


@timing
def update_fip4_daily_table(result_table: str, prod_source_path: str, preprod_source_path: str, chunk=200*2**20):
    yta = YTAdapter().yt
    yql_adapter = YQLAdapter()

    # Check if result table exists and define starting date
    result_table_folder = '/'.join(result_table.split('/')[:-1])
    result_table_name = result_table.split('/')[-1]
    if result_table_name in yta.list(result_table_folder):
        dt_temp = yql_adapter.execute_query(f'SELECT Max(`date`) FROM `{result_table}`', to_pandas=True).iloc[0][0]
        dt_from = (datetime.strptime(dt_temp, '%Y-%m-%d') + timedelta(days=1)).strftime('%Y-%m-%d')
    else:
        min_date_in_prod = min(yta.list(prod_source_path))
        min_date_in_preprod = min(yta.list(preprod_source_path))
        dt_from = max(min_date_in_prod, min_date_in_preprod)

    # Check if new date is available
    if (dt_from not in yta.list(prod_source_path)) or (dt_from not in yta.list(preprod_source_path)):
        logger.info(f"There is no table with name '{dt_from}' in folder '{prod_source_path}' or folder '{preprod_source_path}'")
    else:

        query = yql_adapter.execute_query(dedent(f'''
            PRAGMA yt.InferSchema = '1';
            PRAGMA yt.Pool = 'cloud_analytics_pool';


            $prod_folder = '{prod_source_path}';
            $preprod_folder = '{preprod_source_path}';
            $date_from = '{dt_from}';
            $result_table = '{result_table}';

            DEFINE SUBQUERY $select_format($folder, $metric, $from) AS
                SELECT DISTINCT
                    Unwrap(Unicode::SplitToList(Cast(`resource_id` AS Utf8), '@')[0]) AS `resource`,
                    Unwrap(Unicode::SplitToList(Cast(`resource_id` AS Utf8), '@')[1]) AS `zone`,
                    TableName() AS `date`,
                    NanVL(Cast(`value` AS Double), 0.0) AS `value`
                FROM Range($folder, $from)
                WHERE `metric` = $metric
                    AND `resource_id` IS NOT NULL
                    AND `value` IS NOT NULL
            END DEFINE;

            $res_table =
                SELECT
                    'prod' AS `env`,
                    u.`resource` AS `resource`,
                    u.`zone` AS `zone`,
                    u.`date` AS `date`,
                    Max(u.`value`) AS `used`,
                    Max(t.`value`) AS `total`
                FROM $select_format($prod_folder, 'used', $date_from) u
                LEFT JOIN $select_format($prod_folder, 'total', $date_from) t
                    USING(`resource`, `zone`, `date`)
                GROUP BY u.`resource`, u.`zone`, u.`date`
                UNION ALL
                SELECT
                    'preprod' AS `env`,
                    u.`resource` AS `resource`,
                    u.`zone` AS `zone`,
                    u.`date` AS `date`,
                    Max(u.`value`) AS `used`,
                    Max(t.`value`) AS `total`
                FROM $select_format($preprod_folder, 'used', $date_from) u
                LEFT JOIN $select_format($preprod_folder, 'total', $date_from) t
                    USING(`resource`, `zone`, `date`)
                GROUP BY u.`resource`, u.`zone`, u.`date`;

            $res_table_all_zones =
                SELECT
                    `env`,
                    `date`,
                    `resource`,
                    'all_zones' AS `zone`,
                    Sum(`used`) AS `used`,
                    Sum(`total`) AS `total`
                FROM $res_table
                GROUP BY `env`, `date`, `resource`
                UNION ALL
                SELECT `env`, `date`, `resource`, `zone`, `used`, `total`
                FROM $res_table;

            $res_table_all_resources =
                SELECT
                    `env`,
                    `date`,
                    'all_resources' AS `resource`,
                    `zone`,
                    Sum(`used`) AS `used`,
                    Sum(`total`) AS `total`
                FROM $res_table_all_zones
                GROUP BY `env`, `date`, `zone`
                UNION ALL
                SELECT `env`, `date`, `resource`, `zone`, `used`, `total`
                FROM $res_table_all_zones;

            INSERT INTO $result_table
                SELECT `env`, `date`, `resource`, `zone`, `used`, `total`
                FROM $res_table_all_resources;
        '''))

        YQLAdapter.attach_files(utils.__file__, 'yql', query)
        query.run()
        query.get_results()
        is_success = YQLAdapter.is_success(query)
        if not is_success:
            raise RuntimeError('YQL script is failed')

    yta.transform(result_table, desired_chunk_size=chunk, optimize_for="scan")
    leave_only_last_table(yta, prod_source_path)
    leave_only_last_table(yta, preprod_source_path)
