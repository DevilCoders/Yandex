import json
import click
import logging.config
from textwrap import dedent
from datetime import datetime, timedelta
from clan_tools.data_adapters.YQLAdapter import YQLAdapter
from clan_tools.data_adapters.YTAdapter import YTAdapter
from clan_tools.logging.logger import default_log_config
from clan_tools.utils.timing import timing

logging.config.dictConfig(default_log_config)
logger = logging.getLogger(__name__)


def generate_query(res_table: str, from_date: str) -> str:
    from_month = datetime.strptime(from_date, '%Y-%m-%d').replace(day=1).strftime('%Y-%m-%d')
    return dedent(f"""
        PRAGMA yt.Pool = 'cloud_analytics_pool';
        PRAGMA AnsiInForEmptyOrNullableItemsCollections;

        $take_dt = ($dt) -> (DateTime::FromSeconds(Cast($dt AS Uint32)));
        $format = DateTime::Format('%Y-%m-%d');
        $ba_personal = '//home/cloud_analytics/import/crm/leads/contact_info';
        $result_table = '{res_table}';

        $cloud_bindings = (
            SELECT
                `service_instance_id` AS `cloud_id`,
                Max_By(`billing_account_id`, `start_time`)  AS `billing_account_id`,
            FROM `//home/cloud-dwh/data/prod/ods/billing/service_instance_bindings`
            WHERE `service_instance_type` = 'cloud'
            GROUP BY `service_instance_id`
        );

        $sku_id =
            SELECT
                `sku_id`,
                Pire::Grep("compute")(`name`) AS `is_sku_compute`,
                Pire::Grep("cpu")(`name`) OR Pire::Grep("gpu")(`name`) AS `is_sku_cpu`,
                Pire::Grep("preemptible")(`name`) AS `is_sku_preemptible`
            FROM `//home/cloud-dwh/data/prod/ods/billing/skus`
            WHERE `deprecated` = False
        ;

        $consumption =
            SELECT
                `date` AS `rep_date`,
                `billing_account_id`,
                raw.`cloud_id` AS `cloud_id`,
                Cast(`usage_quantity` AS Double) AS `usage_quantity`,
                Yson::ConvertToString(Yson::ParseJson(`labels_json`)["user_labels"]["managed-kubernetes-cluster-id"]) IS NOT Null AS is_k8s,
                `is_sku_cpu`,
                `is_sku_compute`,
                `is_sku_preemptible`
            FROM Range(`//home/cloud-dwh/data/prod/ods/billing/billing_records/1mo`, `{from_month}`) AS raw
            LEFT JOIN $sku_id AS sk ON raw.`sku_id` = sk.`sku_id`
            WHERE `pricing_unit` = 'core*hour'
        ;

        $billing_usage =
            SELECT
                `rep_date`,
                `billing_account_id`,
                `cloud_id`,
                SUM(CASE WHEN `is_sku_preemptible` THEN `usage_quantity` ELSE 0 END)/100/24 AS `usage_preemptible`,
                SUM(CASE WHEN `is_sku_preemptible` THEN 0 ELSE `usage_quantity` END)/100/24 AS `usage_on_demand`
            FROM $consumption
            WHERE is_k8s = False
                AND `is_sku_compute` = true
                AND `is_sku_cpu` = true
            GROUP BY `rep_date`, `billing_account_id`, `cloud_id`
        ;

        $ba_general =
            SELECT DISTINCT
                `billing_account_id`,
                `person_type_current` AS `person_type`,
                `segment_current` AS `segment`,
                `state_current` AS `state`,
                `usage_status_current` AS `status`,
                String::Strip(`billing_account_name`) AS `ba_name`
            FROM `//home/cloud-dwh/data/prod/cdm/dm_ba_crm_tags`
        ;

        $cloud_info =
            SELECT
                `cloud_id`,
                cb.`billing_account_id` AS `billing_account_id`,
                `last_name` || ' ' || `first_name` AS `display_name`,
                `ba_name`,
                `person_type`,
                `segment`,
                `state`,
                `status`
            FROM $cloud_bindings AS cb
            LEFT JOIN $ba_personal AS bap ON cb.`billing_account_id` = bap.`billing_account_id`
            LEFT JOIN $ba_general AS bag ON cb.`billing_account_id` = bag.`billing_account_id`
        ;

        $cloud_status =
            SELECT `cloud_id`, Max_by(`status`, `modified_at`) AS `cloud_status`
            FROM `//home/cloud-dwh/data/prod/ods/iam/clouds`
            GROUP BY `cloud_id`
        ;

        $quota =
            SELECT DISTINCT
                $take_dt(`timestamp`) AS `rep_datetime`,
                $format($take_dt(`timestamp`)) AS `rep_date`,
                aq.`cloud_id` AS `cloud_id`,
                aq.`quota_name` AS `quota_name`,
                `cloud_status`,
                Coalesce(Cast(aq.`quota_limit` AS Int64), dq.`quota_limit`)/100 AS `quota_limit`,
                Coalesce(Cast(`usage` AS Double), 0.0)/100 AS `usage`
            FROM `//home/cloud-dwh/data/prod/cdm/dm_quota_usage_and_limits` AS aq
            LEFT JOIN `//home/cloud-dwh/data/prod/ods/compute/default_quotas` AS dq ON aq.`quota_name` = dq.`quota_name`
            LEFT JOIN $cloud_status AS cs ON aq.`cloud_id` = cs.`cloud_id`
            WHERE aq.`quota_name` = 'instance-cores'
                AND $format($take_dt(`timestamp`)) > '{from_date}'
                AND $format($take_dt(`timestamp`)) < $format(CurrentUtcDate())
                AND `cloud_status` != 'DELETED'
        ;

        $quota_daily =
            SELECT
                `rep_date`,
                `cloud_id`,
                Some(`cloud_status`) AS `cloud_status`,
                Max(`quota_limit`) AS `quota_limit`,
                Median(`usage`) AS `usage`
            FROM $quota
            GROUP BY `rep_date`, `cloud_id`
        ;

        INSERT INTO $result_table
            SELECT
                qd.`rep_date` AS `rep_date`,
                qd.`cloud_id` AS `cloud_id`,
                ci.`billing_account_id` AS `billing_account_id`,
                `display_name`,
                `ba_name`,
                IF(qd.`cloud_id` IN ('ycmarketplace', 'mdb', 'yc.nbs.tests.cloud'), 'active', `state`) AS `ba_state`,
                IF(qd.`cloud_id` IN ('ycmarketplace', 'mdb', 'yc.nbs.tests.cloud'), 'service', `status`) AS `ba_status`,
                `person_type` AS `ba_person_type`,
                `segment` AS `crm_segment`,
                String::AsciiToLower( `cloud_status`) AS `cloud_status`,
                `quota_limit`,
                `usage`,
                `usage_preemptible` + `usage_on_demand` AS `unit_wheighted_usage`,
                `usage_preemptible` AS `unit_wheighted_usage_preemptible`,
                `usage_on_demand` AS `unit_wheighted_usage_on_demand`
            FROM $quota_daily AS qd
            LEFT JOIN $cloud_info AS ci ON qd.`cloud_id` = ci.`cloud_id`
            LEFT JOIN $billing_usage AS bu ON qd.`rep_date` = bu.`rep_date` AND qd.`cloud_id` = bu.`cloud_id`
            ORDER BY `rep_date`, `cloud_id`, `quota_limit`
    """)


@timing
@click.command()
@click.option('--rebuild', is_flag=True, default=False)
@click.option('--is_prod', is_flag=True, default=False)
@click.option('--table', default='//home/cloud_analytics/ml/capacity_planning/vcpu/top_quotas')
def main(rebuild: bool, is_prod: bool, table: str) -> None:
    yql_adapter = YQLAdapter()
    yt_adapter = YTAdapter()

    table = table if is_prod else table + '_test'
    table_name = table.split('/')[-1]
    table_folder = '/'.join(table.split('/')[:-1])
    table_exists = table_name in yt_adapter.yt.list(table_folder)

    if is_prod:
        if not rebuild and table_exists:
            last_rep_date = yql_adapter.execute_query(f'SELECT Max(rep_date) FROM `{table}`', True).iloc[0, 0]
        else:
            last_rep_date = '2022-01-01'
            if table_exists:
                yt_adapter.yt.remove(table)
    else:
        last_rep_date = (datetime.now() - timedelta(days=5)).strftime('%Y-%m-%d')
        if table_exists:
            yt_adapter.yt.remove(table)

    query = yql_adapter.execute_query(generate_query(table, last_rep_date))

    query.run()
    query.get_results()
    is_success = YQLAdapter.is_success(query)
    if is_success:
        with open('output.json', 'w') as f:
            json.dump({"table_path" : table}, f)


if __name__ == "__main__":
    main()
