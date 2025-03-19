import json
import click
import logging.config
from textwrap import dedent
from clan_tools import utils
from clan_tools.data_adapters.YQLAdapter import YQLAdapter
from clan_tools.logging.logger import default_log_config
from clan_tools.utils.timing import timing

logging.config.dictConfig(default_log_config)
logger = logging.getLogger(__name__)


@timing
@click.command()
@click.option('--forecast_nbs_path', default="//home/cloud_analytics/resources_overbooking/forecast-nbs")
@click.option('--forecast_kkr_nrd_path', default="//home/cloud_analytics/resources_overbooking/forecast-kkr-nrd")
@click.option('--container_limits_path', default="//home/cloud_analytics/resources_overbooking/container_limits")
@click.option('--detailed_nbs_path', default="//home/cloud_analytics/resources_overbooking/nbs")
@click.option('--result_table_path', default="//home/cloud_analytics/resources_overbooking/disk_capacity_monitoring")
def main(forecast_nbs_path, forecast_kkr_nrd_path, container_limits_path, detailed_nbs_path, result_table_path):
    yql_adapter = YQLAdapter()
    query = yql_adapter.execute_query(dedent(f'''
    PRAGMA yt.Pool = 'cloud_analytics_pool';
    PRAGMA AnsiInForEmptyOrNullableItemsCollections;
    PRAGMA yt.InferSchema = '1';

    $result_table = '{result_table_path}';

    $nbs_max_training_date = (
        select
            max(training_date) as max_tr_date
        from `{forecast_nbs_path}`
    );

    $kkr_max_training_date = (
        select
            max(training_date) as max_tr_date
        from `{forecast_kkr_nrd_path}`
    );

    $nbs_table = (
        select
            `env`,
            `disk_type`,
            `datacenter`,
            `date`,
            AVG(IF(`metric` = 'used_TB', `value`, Null)) as `nbs_used`,
            AVG(IF(`metric` = 'purchased_TB', `value`, Null)) as `nbs_purchased`
        from `{forecast_nbs_path}`
        where `training_date` = $nbs_max_training_date
        group by `env`, `disk_type`, `datacenter`, `date`
    );

    $kkr_table = (
        select
            `env`,
            `disk_type`,
            `datacenter`,
            `date`,
            `used_TB` as `kkr_used`,
            `total_TB` as `kkr_total`,
            `comment` as `kkr_comment`,
        from `{forecast_kkr_nrd_path}`
        where `training_date` = $kkr_max_training_date
    );

    $disk_capacity_ratios = (select
            coalesce(kkr.`env`, nbs.`env`) as `env`,
            coalesce(kkr.`disk_type`, nbs.`disk_type`) as `disk_type`,
            coalesce(kkr.`datacenter`, nbs.`datacenter`) as `datacenter`,
            coalesce(kkr.`date`, nbs.`date`) as `date`,
            nbs_used,
            nbs_purchased,
            kkr_used,
            kkr_total,
            kkr_comment,
            nbs_used/nbs_purchased as nbs_ratio,
            kkr_used/nbs_used as kkr_ratio
        from $kkr_table as kkr
            full join $nbs_table as nbs
                on kkr.`env` = nbs.`env`
                and kkr.`disk_type` = nbs.`disk_type`
                and kkr.`datacenter` = nbs.`datacenter`
                and kkr.`date` = nbs.`date`
    );

    insert into $result_table with truncate
    select
        `env`,
        `disk_type`,
        `datacenter`,
        `date`,
        `nbs_used`,
        `nbs_purchased`,
        `kkr_used`,
        `kkr_total`,
        `kkr_comment`,
        `nbs_used`/`nbs_purchased` as `nbs_ratio`,
        `kkr_used`/`nbs_used` as `kkr_ratio`
    from $disk_capacity_ratios
    ;'''))

    YQLAdapter.attach_files(utils.__file__, 'yql', query)
    query.run()
    query.get_results()
    is_success = YQLAdapter.is_success(query)
    if is_success:
        with open('output.json', 'w') as f:
            json.dump({"table_path" : result_table_path}, f)


if __name__ == "__main__":
    main()
