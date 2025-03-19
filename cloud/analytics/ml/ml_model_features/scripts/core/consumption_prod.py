import json
import re
import click
import logging.config
import numpy as np
import pandas as pd
from textwrap import dedent
from clan_tools.data_adapters.YQLAdapter import YQLAdapter
from clan_tools.data_adapters.YTAdapter import YTAdapter
from clan_tools.logging.logger import default_log_config
from yql.client.parameter_value_builder import YqlParameterValueBuilder as ValueBuilder

logging.config.dictConfig(default_log_config)
logger = logging.getLogger(__name__)


def sku_wrapper(name: str, domain: str) -> str:
    """
    Expected behavior:
    SUM(IF(`sku_service_name`=='mdb.dataproc.cpu.c50', COALESCE(`billing_record_total_rub`, 0.0), 0.0))
    """
    assert(domain in ['sku_service_name', 'sku_service_group', 'sku_subservice_name'])
    valid_name = domain + "_" + re.sub('\.', '_', name)
    return f"SUM(IF(`{domain}`=='{name}', COALESCE(`billing_record_total_rub`, 0.0), 0.0)) as `{valid_name}`,\n"


def get_sku_names(yql_adapter: YQLAdapter, domain: str) -> np.ndarray:
    names = yql_adapter.run_query_to_pandas(dedent(f'''
        PRAGMA yt.Pool = 'cloud_analytics_pool';
        Use hahn;
        SELECT DISTINCT `{domain}`
        FROM hahn.`home/cloud-dwh/data/prod/cdm/dm_yc_consumption`
        ''')).iloc[:, 0].dropna().values
    return names


@click.command()
@click.option('--rebuild', is_flag=True, default=False)
def main(rebuild: bool = False) -> None:
    yql_adapter = YQLAdapter()
    yt_adapter = YTAdapter()
    result_folder_path = "//home/cloud_analytics/ml/ml_model_features/by_baid/consumption"

    sku_service_names = get_sku_names(yql_adapter, 'sku_service_name')
    sku_service_groups = get_sku_names(yql_adapter, 'sku_service_group')
    sku_subservice_names = get_sku_names(yql_adapter, 'sku_subservice_name')

    query = yql_adapter._client.query(query=dedent(f'''
    PRAGMA yt.Pool = 'cloud_analytics_pool';
    PRAGMA AnsiInForEmptyOrNullableItemsCollections;
    PRAGMA yt.TmpFolder = '//home/cloud_analytics/ml/ml_model_features/tmp';
    PRAGMA OrderedColumns;

    DECLARE $dates AS List<String>;

    $dm_crm_tags_path = "//home/cloud-dwh/data/prod/cdm/dm_ba_crm_tags";
    $dm_yc_cons_path = "//home/cloud-dwh/data/prod/cdm/dm_yc_consumption";

    DEFINE ACTION $get_data_for_one_date($date) as
        $output = "{result_folder_path}" || "/" || $date;
        $pattern = $date || "-%";

        INSERT INTO $output WITH TRUNCATE
        SELECT
            x.`billing_account_id` as `billing_account_id`,
            `date`,
            COALESCE(SUM(`billing_record_cost_rub`), 0.0) as `billing_record_cost_rub`,
            COALESCE(SUM(`billing_record_credit_rub`), 0.0) as `billing_record_credit_rub`,
            COALESCE(SUM(`billing_record_credit_trial_rub`), 0.0) as `billing_record_credit_trial_rub`,
            COALESCE(SUM(`billing_record_expense_rub`), 0.0) as `billing_record_expense_rub`,
            COALESCE(SUM(`billing_record_total_redistribution_rub`), 0.0) as `billing_record_total_redistribution_rub`,
            COALESCE(SUM(`billing_record_total_rub`), 0.0) as `billing_record_total_rub`,
            COALESCE(SUM(`billing_record_var_reward_rub`), 0.0) as `billing_record_var_reward_rub`,
            COALESCE(SUM(`billing_record_real_consumption_rub`), 0.0) as `billing_record_real_consumption_rub`,
            SUM(IF(`sku_lazy`== 1, COALESCE(`billing_record_total_rub`, 0.0), 0.0)) as `sku_lazy_total_rub`,
            SUM(IF(`sku_lazy`== 0, COALESCE(`billing_record_total_rub`, 0.0), 0.0)) as `sku_non_lazy_total_rub`,
            {"".join([sku_wrapper(name, 'sku_service_name') for name in  sku_service_names])}
            {"".join([sku_wrapper(name, 'sku_service_group') for name in  sku_service_groups])}
            {"".join([sku_wrapper(name, 'sku_subservice_name') for name in  sku_subservice_names])}
        FROM (SELECT `billing_account_id`, `date` FROM $dm_crm_tags_path) as x
        LEFT JOIN $dm_yc_cons_path as y
        ON x.`billing_account_id` == y.`billing_account_id` AND x.`date` == CAST(y.`billing_record_msk_date` as String)
        WHERE `date` LIKE $pattern
        GROUP BY CAST(`billing_record_msk_date` as String) as `date`, x.`billing_account_id`
    END DEFINE;

    EVALUATE FOR $date IN $dates
        DO $get_data_for_one_date($date)
    '''), syntax_version=1)

    month_list = pd.date_range('2019-01-01', 'today', freq='MS').strftime("%Y-%m").tolist()
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

    yql_adapter.is_success(query)
    with open('output.json', 'w+') as f:
        json.dump({"processed_dates" : month_list}, f)


if __name__ == "__main__":
    main()
