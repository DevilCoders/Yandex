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
@click.option('--prod', is_flag=True)
def main(prod):
    yql_adapter = YQLAdapter()
    result_table_path = '//home/cloud_analytics/resources_overbooking/resource_monitoring'
    query = yql_adapter.execute_query(dedent(f'''
    PRAGMA yt.Pool = 'cloud_analytics_pool';
    

    $result_table =  '{result_table_path}';

    

    --sql
    $last_metric = (SELECT
                        node_name,
                        zone_id,
                        platform,
                        start as start_time,
                        metric,
                        `last`
                    FROM RANGE('//home/cloud_analytics/import/resources/1mon/last')
            );


    --sql
    $min_metric = (SELECT
                        node_name,
                        zone_id,
                        platform,
                        start as start_time,
                        metric,
                        `min`
                    FROM RANGE('//home/cloud_analytics/import/resources/1mon/min')
            );
            
    --sql
    $max_metric = (SELECT
                        node_name,
                        zone_id,
                        platform,
                        start as start_time,
                        metric,
                        `max`
                    FROM RANGE('//home/cloud_analytics/import/resources/1mon/max')
            );
            
    --sql
    $avg_metric = (SELECT
                        node_name,
                        zone_id,
                        platform,
                        start as start_time,
                        metric,
                        `avg`
                    FROM RANGE('//home/cloud_analytics/import/resources/1mon/avg')
            );

    --sql
    $sum_metric = (SELECT
                        node_name,
                        zone_id,
                        platform,
                        start as start_time,
                        metric,
                        `sum`
                    FROM RANGE('//home/cloud_analytics/import/resources/1mon/sum')
            ); 



    --sql
    INSERT INTO $result_table WITH TRUNCATE
    SELECT
    *
    FROM $last_metric  AS last_metric

        FULL JOIN $min_metric AS min_metric 
        ON      last_metric.node_name  =  min_metric.node_name  
            AND last_metric.zone_id    =  min_metric.zone_id
            AND last_metric.platform   =  min_metric.platform 
            AND last_metric.start_time =  min_metric.start_time 
            AND last_metric.metric     =  min_metric.metric
        
        FULL JOIN $max_metric AS max_metric 
        ON      last_metric.node_name  =  max_metric.node_name  
            AND last_metric.zone_id    =  max_metric.zone_id
            AND last_metric.platform   =  max_metric.platform 
            AND last_metric.start_time =  max_metric.start_time 
            AND last_metric.metric     =  max_metric.metric
        
        FULL JOIN $avg_metric AS avg_metric 
        ON      last_metric.node_name  =  avg_metric.node_name  
            AND last_metric.zone_id    =  avg_metric.zone_id
            AND last_metric.platform   =  avg_metric.platform 
            AND last_metric.start_time =  avg_metric.start_time 
            AND last_metric.metric     =  avg_metric.metric
            
            
        FULL JOIN $sum_metric AS sum_metric 
        ON      last_metric.node_name  =  sum_metric.node_name 
            AND last_metric.zone_id    =  sum_metric.zone_id
            AND last_metric.platform   =  sum_metric.platform 
            AND last_metric.start_time =  sum_metric.start_time
            AND last_metric.metric     =  sum_metric.metric
        
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