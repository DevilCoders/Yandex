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
    result_table_path = '//home/cloud_analytics/dashboards/CLOUDBIZ-4563-key-partners/account_owners'
    query = yql_adapter.execute_query(dedent(f'''
    PRAGMA yt.Pool = 'cloud_analytics_pool';
    PRAGMA Library('tables.sql');
    
    IMPORT tables SYMBOLS $last_non_empty_table;

    $result_table_path = '{result_table_path}';
    $accounts = '//home/cloud_analytics/dwh/raw/crm/accounts';
    $accountroles = '//home/cloud_analytics/dwh/raw/crm/accountroles';
    $billingaccounts = '//home/cloud_analytics/dwh/raw/crm/billingaccounts';
    $users = '//home/cloud_analytics/dwh/raw/crm/users';
    --sql
    insert into $result_table_path with truncate
    SELECT
        ba.name AS billing_account_id,
        u.user_name AS account_owner,
        acc.name AS account_name

    FROM $last_non_empty_table($accounts) AS acc
    LEFT JOIN $last_non_empty_table($accountroles) AS roles
        ON acc.id = roles.account_id
    LEFT JOIN $last_non_empty_table($billingaccounts) AS ba
        ON acc.id = ba.account_id
    LEFT JOIN $last_non_empty_table($users) AS u 
        ON u.id = roles.assigned_user_id
    WHERE  
        acc.deleted = False  
        AND
        roles.role = 'account_owner' AND roles.deleted = False
        AND roles.status in ('confirmed', 'pending_recycling')
        AND u.deleted = False
        AND ba.deleted = False
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