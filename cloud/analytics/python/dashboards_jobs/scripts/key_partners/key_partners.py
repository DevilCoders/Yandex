import json
import logging.config
from textwrap import dedent
import click
from clan_tools import utils
from clan_tools.data_adapters.YQLAdapter import YQLAdapter
from clan_tools.utils.conf import read_conf
from clan_tools.utils.timing import timing

config = read_conf('config/logger.yml')
logging.config.dictConfig(config)
logger = logging.getLogger(__name__)

@timing
@click.command()
@click.option('--write', is_flag=True)
def main(write):
    yql_adapter = YQLAdapter()
    result_table_path = '//home/cloud_analytics/dashboards/CLOUDBIZ-4563-key-partners/key_partners_plans'
    query = yql_adapter.execute_query(dedent(f'''
    PRAGMA yt.Pool = 'cloud_analytics_pool';
    PRAGMA Library('tables.sql');
    
    IMPORT tables SYMBOLS $last_non_empty_table;

    $result_table_path = '{result_table_path}';
    $accounts = '//home/cloud_analytics/dwh/raw/crm/accounts';
    $billingaccounts = '//home/cloud_analytics/dwh/raw/crm/billingaccounts';
    $tag_bean_rel = '//home/cloud_analytics/dwh/raw/crm/tag_bean_rel';
    $tags = '//home/cloud_analytics/dwh/raw/crm/tags';
    $plans = '//home/cloud_analytics/dwh/raw/crm/plans';
    $timeperiods = '//home/cloud_analytics/dwh/raw/crm/timeperiods';
    $dimensions = '//home/cloud_analytics/dwh/cdm/dim_crm_account';


    --sql
    insert into $result_table_path with truncate
    select distinct
           acc.name as account_name, 
           b.name as billing_account_id,  
           p.value_currency as plan,
           timeperiods.name as plan_timeperiod_name,
           cast(start_date_timestamp as Uint32) as plan_time_start,
           cast(end_date_timestamp as Uint32) as plan_time_end,
           t.name as tag
    from $last_non_empty_table($accounts) as acc
    left join $last_non_empty_table($billingaccounts) as b
        on b.account_id = acc.id
    left join $last_non_empty_table($tag_bean_rel) as t_b
        on acc.id = t_b.bean_id
    left join $last_non_empty_table($tags) as t
        on t_b.tag_id = t.id
    left join $last_non_empty_table($plans) as p
        on p.account_id = acc.id
    left join $last_non_empty_table($timeperiods) as timeperiods
        on p.selectedTimePeriod = timeperiods.id
    where (String::ToLower(t.name) in ('key_var', 'speechkit pro')) 
        and (t_b.deleted = false) and (p.deleted = false)
    --endsql
    '''))

    YQLAdapter.attach_files(utils.__file__, 'yql', query)
    query.run()
    query.get_results()
    is_success = YQLAdapter.is_success(query)
    if is_success:
         with open('output.json', 'w') as f:
            json.dump({"table_path" : result_table_path }, f)


  
    


if __name__ == "__main__":
    main()