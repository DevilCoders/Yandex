import json
import logging.config
from textwrap import dedent
from clan_tools import utils
from clan_tools.data_adapters.YQLAdapter import YQLAdapter
from clan_tools.logging.logger import default_log_config
from clan_tools.utils.timing import timing

logging.config.dictConfig(default_log_config)
logger = logging.getLogger(__name__)


@timing
def main():
    yql_adapter = YQLAdapter()
    result_table_path = '//home/cloud_analytics/resources_overbooking/preemptible_cores'

    query = yql_adapter.execute_query(dedent(f'''
    PRAGMA yt.Pool = 'cloud_analytics_pool';
    PRAGMA AnsiInForEmptyOrNullableItemsCollections;

    $result_table =  '{result_table_path}';

    $toDateTime = ($dt) -> {{
        return cast(cast($dt as uint64) as datetime)
    }};
    $toDate = ($dt) -> {{
        return cast($toDateTime($dt) as date)
    }};
    $table_with_preems = (
        select
            'all_nodes' as `node_name`,
            `zone_id`,
            `platform`,
            `start_time`,
            $toDateTime(`start_time`) as `dtt`,
            $toDate(`start_time`) as `start_date`,
            Sum(`min`) as `min_preems`
        from `home/cloud_analytics/resources_overbooking/resource_monitoring`
        where `node_name` != 'all_nodes'
            and `metric` like '%preemptible%core%'
        group by `zone_id`, `platform`, `start_time`
    );

    $preem_cores = (
        select
            `start_date`,
            `platform`,
            `node_name`,
            `zone_id`,
            Min(`min_preems`) as `daily_min_preems`
        from $table_with_preems
        group by `node_name`, `zone_id`, `platform`, `start_date`
    );
    $preem_cores_all = (
        select
            `start_date`,
            `platform`,
            `node_name`,
            'all_zones' as `zone_id`,
            sum(`daily_min_preems`) as `daily_min_preems`
        from $preem_cores
        group by `start_date`, `platform`, `node_name`
    );
    $preems_zones = (
        select * from $preem_cores_all
        union all
        select * from $preem_cores
    );
    $preem_platforms_all = (
        select
            `start_date`,
            `node_name`,
            `zone_id`,
            'all_platforms' as `platform`,
            sum(`daily_min_preems`) as `daily_min_preems`
        from $preem_cores
        group by `start_date`, `node_name`, `zone_id`
    );
    $preem_platforms_all_cpu = (
        select
            `start_date`,
            `node_name`,
            `zone_id`,
            'all_cpu_platforms' as `platform`,
            sum(`daily_min_preems`) as `daily_min_preems`
        from $preem_cores
        where `platform` in ('standard-v1', 'standard-v2', 'standard-v3', 'unknown')
        group by `start_date`, `node_name`, `zone_id`
    );
    $preem_platforms_all_gpu = (
        select
            `start_date`,
            `node_name`,
            `zone_id`,
            'all_gpu_platforms' as `platform`,
            sum(`daily_min_preems`) as `daily_min_preems`
        from $preem_cores
        where `platform` not in ('standard-v1', 'standard-v2', 'standard-v3', 'unknown')
        group by `start_date`, `node_name`, `zone_id`
    );
    $preems = (
        select * from $preem_platforms_all
        union all
        select * from $preem_platforms_all_cpu
        union all
        select * from $preem_platforms_all_gpu
        union all
        select * from $preems_zones
    );
    insert into $result_table with truncate
    select * from $preems
    ;
    '''))

    YQLAdapter.attach_files(utils.__file__, 'yql', query)
    query.run()
    query.get_results()
    is_success = YQLAdapter.is_success(query)
    if is_success:
        with open('output.json', 'w') as f:
            json.dump({"table_path" : result_table_path}, f)


if __name__ == "__main__":
    main()
