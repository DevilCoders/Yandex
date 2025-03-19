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
    result_table_path = '//home/cloud_analytics/import/network-logs/autoabuse/latest_time'
    query = yql_adapter.execute_query(dedent(f'''
    PRAGMA yt.Pool='cloud_analytics_pool';

    $result_table = '//home/cloud_analytics/import/network-logs/autoabuse/ba_ips_ssh_rdp';
    $result_table_latest_time = 
        '{result_table_path}';


    $format = DateTime::Format("%Y-%m-%dT%H:%M:%S");
    --sql
    INSERT INTO $result_table_latest_time WITH TRUNCATE 
    SELECT $format(MAX(max_setup_time) + Interval("PT3H")) AS latest_time
    FROM $result_table;
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