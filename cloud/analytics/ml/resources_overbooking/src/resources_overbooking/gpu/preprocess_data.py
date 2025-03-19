import logging.config
from textwrap import dedent
from clan_tools.data_adapters.YQLAdapter import YQLAdapter
from clan_tools.data_adapters.YTAdapter import YTAdapter
from clan_tools.logging.logger import default_log_config
from datetime import datetime, timedelta
from clan_tools.utils.timing import timing

logging.config.dictConfig(default_log_config)
logger = logging.getLogger(__name__)


@timing
def update_gpu_daily_table(result_table: str, prod_source_path: str, preprod_source_path: str, chunk=200*2*20):
    yt_adapter = YTAdapter()
    yta = yt_adapter.yt
    yql_adapter = YQLAdapter()

    result_table_folder = '/'.join(result_table.split('/')[:-1])
    result_table_name = result_table.split('/')[-1]
    if result_table_name in yta.list(result_table_folder):
        dt_temp = yql_adapter.run_query_to_pandas(f'SELECT Max(`date`) FROM `{result_table}`').iloc[0][0]
        dt_from = (datetime.strptime(dt_temp, '%Y-%m-%d') + timedelta(days=1)).strftime('%Y-%m-%d')
    else:
        min_date_in_prod = min(yta.list(prod_source_path))
        min_date_in_preprod = min(yta.list(preprod_source_path))
        dt_from = max(min_date_in_prod, min_date_in_preprod)

    if dt_from not in yta.list(prod_source_path):
        logger.info(f"There is no table with name '{dt_from}' in folder '{prod_source_path}'")
        return None
    if dt_from not in yta.list(preprod_source_path):
        logger.info(f"There is no table with name '{dt_from}' in folder '{preprod_source_path}'")
        return None

    query = yql_adapter.run_query(dedent(f'''
        PRAGMA yt.InferSchema = '1';
        PRAGMA yt.Pool = 'cloud_analytics_pool';

        $prod_folder = '{prod_source_path}';
        $preprod_folder = '{preprod_source_path}';
        $date_from = '{dt_from}';
        $result_table = '{result_table}';

        DEFINE SUBQUERY $select_format($folder, $metric, $from) AS
            SELECT DISTINCT
                Unwrap(`model`) AS `model`,
                Unwrap(`platform`) AS `platform`,
                Unwrap(`zone_id`) AS `zone`,
                TableName() AS `date`,
                NanVL(Cast(`value` AS Double), 0.0) AS `value`
            FROM Range($folder, $from)
            WHERE `metric` = $metric
                AND `model` IS NOT NULL
                AND `platform` IS NOT NULL
                AND `zone_id` IS NOT NULL
                AND `value` IS NOT NULL
        END DEFINE;

        $gpu_prod_free =
            SELECT 'prod' AS `env`, `model`, `platform`, `zone`, `date`, Median(`value`) AS `free`
            FROM $select_format($prod_folder, 'gpu_free', $date_from)
            WHERE `value` > 0
            GROUP BY `model`, `platform`, `zone`, `date`;

        $gpu_prod_total =
            SELECT 'prod' AS `env`, `model`, `platform`, `zone`, `date`, Median(`value`) AS `total`
            FROM $select_format($prod_folder, 'gpu_total', $date_from)
            WHERE `value` > 0
            GROUP BY `model`, `platform`, `zone`, `date`;

        $gpu_preprod_free =
            SELECT 'preprod' AS `env`, `model`, `platform`, `zone`, `date`, Median(`value`) AS `free`
            FROM $select_format($preprod_folder, 'gpu_free', $date_from)
            WHERE `value` > 0
            GROUP BY `model`, `platform`, `zone`, `date`;

        $gpu_preprod_total =
            SELECT 'preprod' AS `env`, `model`, `platform`, `zone`, `date`, Median(`value`) AS `total`
            FROM $select_format($preprod_folder, 'gpu_total', $date_from)
            WHERE `value` > 0
            GROUP BY `model`, `platform`, `zone`, `date`;

        $res_table =
            SELECT
                Coalesce(pf.`env`, pt.`env`) AS `env`,
                Coalesce(pf.`model`, pt.`model`) AS `model`,
                Coalesce(pf.`platform`, pt.`platform`) AS `platform`,
                Coalesce(pf.`zone`, pt.`zone`) AS `zone`,
                Coalesce(pf.`date`, pt.`date`) AS `date`,
                pt.`total` - pf.`free` AS `used`,
                pt.`total` AS `total`
            FROM $gpu_prod_free AS pf
            FULL JOIN $gpu_prod_total AS pt USING(`env`, `model`, `platform`, `zone`, `date`)
            UNION ALL
            SELECT
                Coalesce(prf.`env`, prt.`env`) AS `env`,
                Coalesce(prf.`model`, prt.`model`) AS `model`,
                Coalesce(prf.`platform`, prt.`platform`) AS `platform`,
                Coalesce(prf.`zone`, prt.`zone`) AS `zone`,
                Coalesce(prf.`date`, prt.`date`) AS `date`,
                prt.`total` - prf.`free` AS `used`,
                prt.`total` AS `total`
            FROM $gpu_preprod_free AS prf
            FULL JOIN $gpu_preprod_total AS prt USING(`env`, `model`, `platform`, `zone`, `date`);

        INSERT INTO $result_table
            SELECT `env`, `date`, `model`, `platform`, `zone`, `used`, `total`
            FROM $res_table;
    '''))

    query.run()
    query.get_results()
    is_success = YQLAdapter.is_success(query)
    if not is_success:
        raise RuntimeError('YQL script is failed')

    yt_adapter.optimize_chunk_number(result_table, optimize_for="scan")
    yt_adapter.leave_last_N_tables(prod_source_path, 1)
    yt_adapter.leave_last_N_tables(preprod_source_path, 1)
