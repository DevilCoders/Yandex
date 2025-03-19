import logging.config
from textwrap import dedent
from clan_tools import utils
from clan_tools.data_adapters.YQLAdapter import YQLAdapter
from clan_tools.data_adapters.YTAdapter import YTAdapter
from clan_tools.logging.logger import default_log_config
from clan_tools.utils.timing import timing
from datetime import datetime

logging.config.dictConfig(default_log_config)
logger = logging.getLogger(__name__)


@timing
def update_fip4_daily_from_consumption(result_table_path: str, chunk: int = 200*2**20):
    yta = YTAdapter().yt
    yql_adapter = YQLAdapter()

    # Check if result table exists and define starting date
    result_table_folder = '/'.join(result_table_path.split('/')[:-1])
    result_table_name = result_table_path.split('/')[-1]
    if result_table_name in yta.list(result_table_folder):
        last_date = yql_adapter.execute_query(f'SELECT Max(`calc_date`) FROM `{result_table_path}`', to_pandas=True).iloc[0][0]
        if last_date == datetime.now().strftime('%Y-%m-%d'):
            logger.info('Table is up to date')
            return None
    else:
        logger.info('Creating new table')

    query = yql_adapter.execute_query(dedent(f"""
        PRAGMA AnsiInForEmptyOrNullableItemsCollections;

        $res_table_path = '{result_table_path}';
        $rep_date = DateTime::Format("%Y-%m-%d")(CurrentTzTimeStamp("Europe/Moscow"));

        $consumption =
            SELECT
                billing_record_msk_date,
                billing_account_id,
                crm_segment,
                billing_record_pricing_quantity,
                billing_account_state,
                billing_account_usage_status,
                billing_record_real_consumption_rub_vat
            FROM `//home/cloud-dwh/data/prod/cdm/dm_yc_consumption`
            WHERE sku_name IN ('network.public_fips',
                            'network.public_fips.lb',
                            'mdb.cluster.mongo.public_ip',
                            'mdb.cluster.clickhouse.public_ip',
                            'mdb.cluster.mysql.public_ip',
                            'mdb.cluster.postgresql.public_ip')
            AND billing_record_msk_date < $rep_date
            AND billing_account_is_suspended_by_antifraud_current = false
            AND billing_account_state IN ('active', 'payment_required', 'suspended')
            AND billing_account_usage_status IN ('trial', 'paid', 'service')
        ;

        $res_table =
            SELECT
                $rep_date AS calc_date,
                billing_record_msk_date AS rep_date,
                crm_segment,
                billing_account_state,
                billing_account_usage_status,
                sum(billing_record_pricing_quantity)/24 AS ip_count,
                sum(billing_record_real_consumption_rub_vat) AS real_consumption_vat
            FROM $consumption
            GROUP BY billing_record_msk_date, crm_segment, billing_account_state, billing_account_usage_status
        ;

        INSERT INTO $res_table_path
            SELECT *
            FROM $res_table
    """))

    YQLAdapter.attach_files(utils.__file__, 'yql', query)
    query.run()
    query.get_results()
    is_success = YQLAdapter.is_success(query)
    if not is_success:
        raise RuntimeError('YQL script is failed')

    yta.transform(result_table_path, desired_chunk_size=chunk, optimize_for="scan")
