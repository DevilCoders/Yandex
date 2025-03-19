import json
import logging.config
from os import name
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
    result_table_path = '//home/cloud_analytics/dashboards/CLOUDBIZ-4563-key-partners/key_partners_opportunities'
    query = yql_adapter.execute_query(dedent(f'''
    PRAGMA yt.Pool = 'cloud_analytics_pool';
    PRAGMA Library('tables.sql');
    
    IMPORT tables SYMBOLS $last_non_empty_table;

    $result_table_path = '{result_table_path}';
    $accounts = '//home/cloud_analytics/dwh/raw/crm/accounts';
    $accounts_opportunities = '//home/cloud_analytics/dwh/raw/crm/accounts_opportunities';
    $opportunities = '//home/cloud_analytics/dwh/raw/crm/opportunities';
    $billingaccounts = '//home/cloud_analytics/dwh/raw/crm/billingaccounts';
    $tag_bean_rel = '//home/cloud_analytics/dwh/raw/crm/tag_bean_rel';
    $tags = '//home/cloud_analytics/dwh/raw/crm/tags';
    $users = '//home/cloud_analytics/dwh/raw/crm/users';
  
    --sql
    insert into $result_table_path with truncate
    select distinct acc1.name as partner_name, 
                    b1.name as partner_ba,
                    acc_plus_partner.acc_name as account_name, 
                    acc_plus_partner.ba as billing_account,
                    acc_plus_partner.partners_value as partners_value,
                    acc_plus_partner.expected_opp_date_closed as expected_opp_close_date,
                    acc_plus_partner.opp_probability as opp_probability,
                    acc_plus_partner.opp_likely as opp_likely,
                    acc_plus_partner.opp_name as opp_name,
                    acc_plus_partner.opp_id as opp_id,
                    acc_plus_partner.opp_sales_stage as opp_sales_stage,
                    acc_owners.account_owner as account_owner,
                    u.user_name AS assigned_user,
                    t1.name as tag
                
    from $last_non_empty_table($accounts) as acc1
    inner join 
        (select acc.name as acc_name, b.name as ba, 
                opp.partner_id as partner_id, 
                opp.partners_value as partners_value,
                opp.date_closed as expected_opp_date_closed,
                opp.probability_enum as opp_probability,
                opp.amount as opp_likely,
                opp.name as opp_name,
                opp.id as opp_id,
                opp.assigned_user_id as opp_assigned_user_id,
                opp.sales_stage as opp_sales_stage


        from $last_non_empty_table($accounts) as acc
        left join $last_non_empty_table($billingaccounts) as b
            on b.account_id = acc.id
        left join $last_non_empty_table($accounts_opportunities) as  ao
            on ao.account_id = acc.id
        left join $last_non_empty_table($opportunities) as opp
            on ao.opportunity_id = opp.id
        where opp.partner_id is not null and opp.deleted=False) as acc_plus_partner
        on acc_plus_partner.partner_id = acc1.id
    left join $last_non_empty_table($billingaccounts) as b1
        on b1.account_id = acc1.id
    left join $last_non_empty_table($tag_bean_rel) as t_b1
        on acc1.id = t_b1.bean_id
    left join $last_non_empty_table($tags) as t1
        on t_b1.tag_id = t1.id
    left join `//home/cloud_analytics/dashboards/CLOUDBIZ-4563-key-partners/account_owners` as acc_owners
         on acc_plus_partner.ba = acc_owners.billing_account_id
    LEFT JOIN $last_non_empty_table($users) AS u 
        ON u.id = acc_plus_partner.opp_assigned_user_id

    where   (String::ToLower(t1.name) in ('key_var', 'speechkit pro')) and acc1.deleted = False
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