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
    result_table_path = '//home/cloud_analytics/dashboards/CLOUDANA-1117-var-partners-managers/var_dashboard'
    query = yql_adapter.execute_query(dedent(f'''
    PRAGMA yt.Pool = 'cloud_analytics_pool';
    PRAGMA Library('tables.sql');

    IMPORT tables SYMBOLS $last_non_empty_table;

    $result_table_path = '//home/cloud_analytics/dwh/cdm/dim_crm_account';
    $accounts = '//home/cloud_analytics/dwh/raw/crm/accounts';
    $billingaccounts = '//home/cloud_analytics/dwh/raw/crm/billingaccounts';
    $dimensions_bean_rel = '//home/cloud_analytics/dwh/raw/crm/dimensions_bean_rel';
    $dimensions = '//home/cloud_analytics/dwh/raw/crm/dimensions';

    --sql
    INSERT INTO $result_table_path WITH TRUNCATE
    SELECT
        DISTINCT
        ba.name AS billing_account_id,
        acc.name AS account_name,
        d.name AS dimension_name

    FROM $last_non_empty_table($accounts) AS acc
    LEFT JOIN $last_non_empty_table($billingaccounts) AS ba
        ON acc.id = ba.account_id
    LEFT JOIN $last_non_empty_table($dimensions_bean_rel) AS d_b
        ON acc.id = d_b.bean_id -- AND d_b.bean_module = 'Leads'
    LEFT JOIN $last_non_empty_table($dimensions) AS d
        ON d_b.dimension_id = d.id
    WHERE  
        acc.deleted = False 
        AND d_b.deleted = False 
        AND d.deleted = False 
        --AND d.name = 'SpeechKit Pro'
        AND ba.name IS NOT NULL;
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