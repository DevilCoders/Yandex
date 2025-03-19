import json
import click
import logging.config
from textwrap import dedent
from clan_tools import utils
from datetime import datetime, timedelta
from clan_tools.data_adapters.YQLAdapter import YQLAdapter
from clan_tools.data_adapters.YTAdapter import YTAdapter
from clan_tools.logging.logger import default_log_config
from clan_tools.utils.timing import timing

logging.config.dictConfig(default_log_config)
logger = logging.getLogger(__name__)


@timing
@click.command()
@click.option('--env_flag', is_flag=True, default=True)
@click.option('--detailed_nbs_path', default="//home/cloud_analytics/resources_overbooking/nbs")
@click.option('--result_table', default="//home/cloud_analytics/resources_overbooking/monitoring_quota_and_nbs")
def main(env_flag, detailed_nbs_path, result_table, chunk_size=100 * 2**20):
    yql_adapter = YQLAdapter()
    yt_adapter = YTAdapter()

    # env: prod / preprod
    env = 'prod' if env_flag else 'preprod'
    result_table = f'{result_table}_{env}'

    # check conditions for starting script
    result_table_name = result_table.split('/')[-1]
    result_table_path = '/'.join(result_table.split('/')[:-1])
    if result_table_name not in yt_adapter.yt.list(result_table_path):
        date_begin = '2021-06-01'
    else:
        date_begin = yql_adapter.execute_query(f'SELECT Max(report_date) FROM `{result_table}`', to_pandas=True).iloc[0, 0]
        date_begin = (datetime.strptime(date_begin, "%Y-%m-%d")+timedelta(days=1)).strftime('%Y-%m-%d')

    date_end = (datetime.today()+timedelta(days=-1)).strftime("%Y-%m-%d")
    if date_begin > date_end:
        logger.info(f'Table is up to date for {date_end}')
        with open('output.json', 'w') as f:
            json.dump({"resul_table" : f'Table is up to date {result_table}'}, f)
        return None

    nbs_table_begin = date_begin.replace('-', '')
    nbs_table_end = min(max(yt_adapter.yt.list(detailed_nbs_path)), date_end.replace('-', ''))

    if nbs_table_begin not in yt_adapter.yt.list(detailed_nbs_path):
        logger.warning(f'New NBS tables are not loaded yet. Looking for {nbs_table_begin}...')
        return None

    query = yql_adapter.execute_query(dedent(f"""
    -----------------------------------------------------------------------------------
    -- Define configuration and functions for script

    PRAGMA yt.InferSchema = '1';
    PRAGMA yt.Pool = 'cloud_analytics_pool';
    PRAGMA AnsiInForEmptyOrNullableItemsCollections;

    $cast_TB = ($x) -> {{
        RETURN Cast($x AS Double)/1024/1024/1024/1024
    }};

    $normal_name = ($str) -> {{
        RETURN Substring(Cast($str AS String), 0, Length($str)-Cast(16 AS Uint32))
    }};

    $take_nbs_date = () -> {{
        $year = Substring(TableName(), 0, 4);
        $month = Substring(TableName(), 4, 2);
        $day = Substring(TableName(), 6, 2);
        RETURN $year || '-' || $month || '-' || $day
    }};

    $take_quota_date = ($dt) -> {{
        RETURN DateTime::Format('%Y-%m-%d')(Cast($dt AS Date))
    }};

    -----------------------------------------------------------------------------------
    -- Extract Cloud ID's names

    $cloud_bindings = (
        SELECT
            `service_instance_id` AS `cloud_id`,
            Max_By(`billing_account_id`, `start_time`)  AS `billing_account_id`,
        FROM `//home/cloud-dwh/data/{env}/ods/billing/service_instance_bindings`
        WHERE `service_instance_type` = 'cloud'
        GROUP BY `service_instance_id`
    );

    $ba_info = (
        SELECT DISTINCT
            b.`cloud_id` AS `cloud_id`,
            b.`billing_account_id` AS `billing_account_id`,
            Max_By(ba.`state`, ba.`updated_at`) AS `state`,
            Max_By(ba.`usage_status`, ba.`updated_at`) AS `status`,
            Max_By(ba.`name`, ba.`updated_at`) AS `ba_name`,
        FROM $cloud_bindings AS b
            LEFT JOIN `//home/cloud-dwh/data/{env}/ods/billing/billing_accounts` AS ba
                ON b.`billing_account_id` = ba.`billing_account_id`
        GROUP BY b.`cloud_id`, b.`billing_account_id`
    );

    $display_names = (
        SELECT
            `billing_account_id`,
            Max_By(`account_display_name`, `billing_record_msk_date`) AS `display_name`
        FROM `//home/cloud-dwh/data/{env}/cdm/dm_yc_consumption`
        GROUP BY `billing_account_id`
    );

    $cloud_names = (
        SELECT
            `cloud_id`,
            cn.`billing_account_id` AS `billing_account_id`,
            `display_name`,
            `ba_name`,
            `state`,
            `status`
        FROM $ba_info AS cn
            LEFT JOIN $display_names AS dn ON cn.`billing_account_id` = dn.`billing_account_id`
    );

    -----------------------------------------------------------------------------------
    -- Extract info about purchased and ised in every zone for all available Cloud IDs

    $nbs_detailed = (
        SELECT
            $take_nbs_date() AS `report_date`,
            `disk_type`,
            `cloud_id`,
            `datacenter`,
            `disk_id`,
            `bytes_used`,
            `bytes_total`
        FROM Range(`{detailed_nbs_path}`, `{nbs_table_begin}`, `{nbs_table_end}`)
        WHERE `cloud_id` != 'yc.disk-manager.cloud' AND `cloud_id` != ''
    );

    $nbs_daily = (
        SELECT
            `report_date`,
            `disk_type`,
            `cloud_id`,
            `datacenter`,
            `disk_id`,
            Max($cast_TB(`bytes_total`)) AS `purchased_TB`,
            Max($cast_TB(`bytes_used`)) AS `used_TB`
        FROM $nbs_detailed
        GROUP BY `report_date`, `disk_type`, `cloud_id`, `datacenter`, `disk_id`
    );

    $nbs_clouds = (
        SELECT
            `report_date`,
            `disk_type`,
            `cloud_id`,
            `datacenter`,
            Sum(`purchased_TB`) AS `purchased_TB`,
            Sum(`used_TB`) AS `used_TB`
        FROM $nbs_daily
        GROUP BY `report_date`, `disk_type`, `cloud_id`, `datacenter`
    );

    $nbs_clouds_by_zone = (
        SELECT
            `report_date`,
            `disk_type`,
            `cloud_id`,
            Sum(IF(`datacenter`='vla', `used_TB`, 0)) AS `vla_used_TB`,
            Sum(IF(`datacenter`='sas', `used_TB`, 0)) AS `sas_used_TB`,
            Sum(IF(`datacenter`='myt', `used_TB`, 0)) AS `myt_used_TB`,
            Sum(IF(`datacenter`='vla', `purchased_TB`, 0)) AS `vla_purchased_TB`,
            Sum(IF(`datacenter`='sas', `purchased_TB`, 0)) AS `sas_purchased_TB`,
            Sum(IF(`datacenter`='myt', `purchased_TB`, 0)) AS `myt_purchased_TB`,
            Sum(`used_TB`) AS `total_used_TB`,
            Sum(`purchased_TB`) AS `total_purchased_TB`
        FROM $nbs_clouds
        GROUP BY `report_date`, `disk_type`, `cloud_id`
    );

    -----------------------------------------------------------------------------------
    -- Extract info about limits and purchased for all available Cloud IDs

    $quota_init = (
        SELECT
            $take_quota_date(`timestamp`) AS `report_date`,
            $normal_name(`quota_name`) AS `disk_type`,
            `cloud_id`,
            $cast_TB(`quota_limit`) AS `given_quota_TB`,
            $cast_TB(`usage`) AS `total_purchased_TB_lim`
        FROM `//home/cloud-dwh/data/{env}/cdm/dm_quota_usage_and_limits`
        WHERE Cast(`timestamp` AS Date) >= Cast('{date_begin}' AS Date)
            AND Cast(`timestamp` AS Date) <= Cast('{date_end}' AS Date)
            AND `quota_name` IN ('network-hdd-total-disk-size', 'network-ssd-nonreplicated-total-disk-size', 'network-ssd-total-disk-size')
    );

    $quota_daily = (
        SELECT
            `report_date`,
            `disk_type`,
            `cloud_id`,
            Max(`given_quota_TB`) AS `given_quota_TB`,
            Max(`total_purchased_TB_lim`) AS `total_purchased_TB_lim`
        FROM $quota_init
        GROUP BY `report_date`, `cloud_id`, `disk_type`
    );

    -----------------------------------------------------------------------------------
    -- Update all info in result table with ranks

    INSERT INTO `{result_table}`
    SELECT
        nbs.`report_date` AS `report_date`,
        nbs.`disk_type` AS `disk_type`,
        nbs.`cloud_id` AS `cloud_id`,
        `billing_account_id`,
        `display_name`,
        `ba_name`,
        `state`,
        `status`,
        `given_quota_TB`,
        `vla_purchased_TB`,
        `sas_purchased_TB`,
        `myt_purchased_TB`,
        `total_purchased_TB`,
        `total_purchased_TB_lim`,
        `vla_used_TB`,
        `sas_used_TB`,
        `myt_used_TB`,
        `total_used_TB`,
        `given_quota_TB` - `total_used_TB` AS `free_quota_TB`
    FROM $nbs_clouds_by_zone AS nbs
        LEFT JOIN $quota_daily AS lmt
            ON nbs.`report_date` = lmt.`report_date`
            AND nbs.`disk_type` = lmt.`disk_type`
            AND nbs.`cloud_id` = lmt.`cloud_id`
        LEFT JOIN $cloud_names AS cln
            ON nbs.`cloud_id` = cln.`cloud_id`
    ;"""))

    YQLAdapter.attach_files(utils.__file__, 'yql', query)
    query.run()
    query.get_results()
    is_success = YQLAdapter.is_success(query)
    if is_success:
        yt_adapter.yt.transform(result_table, desired_chunk_size=chunk_size)
        with open('output.json', 'w') as f:
            json.dump({"resul_table" : result_table}, f)


if __name__ == "__main__":
    main()
