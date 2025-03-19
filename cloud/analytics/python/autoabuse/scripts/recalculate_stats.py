import json
import logging.config
from textwrap import dedent
import click
from clan_tools import utils
from clan_tools.data_adapters.YQLAdapter import YQLAdapter
from clan_tools.logging.logger import default_log_config
from clan_tools.utils.timing import timing

logging.config.dictConfig(default_log_config)
logger = logging.getLogger(__name__)

@timing
@click.command()
@click.option('--write', is_flag=True)
def main(write):
    yql_adapter = YQLAdapter()
    result_table_path = '//home/cloud_analytics/import/network-logs/autoabuse/ba_ips_ssh_rdp'
    query = yql_adapter.execute_query(dedent(f'''
    PRAGMA yt.Pool='cloud_analytics_pool';
    PRAGMA Library('network_stats.sql');
    
    IMPORT network_stats SYMBOLS $stats;

    $result_table = '{result_table_path}';
    
    --sql
    INSERT INTO $result_table
    SELECT *
    FROM $stats('//logs/yc-antifraud-overlay-flows-stats/1d', '');


   
    '''))

    req = YQLAdapter.attach_files(utils.__file__, 'yql', query)
    req.attach_file('scripts/yql/network_stats.sql', 'network_stats.sql')

    query.run()
    query.get_results()
    is_success = YQLAdapter.is_success(query)
    if is_success:
         with open('output.json', 'w') as f:
            json.dump({"table_path" : result_table_path }, f)


  
    


if __name__ == "__main__":
    main()