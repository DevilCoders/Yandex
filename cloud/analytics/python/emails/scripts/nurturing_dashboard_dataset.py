from textwrap import dedent

from pandas.io import sql
from clan_tools.data_adapters.YQLAdapter import YQLAdapter
import logging.config
from clan_tools.utils.conf import read_conf
from clan_tools.utils.timing import timing
import click
from yql.client.operation import YqlSqlOperationRequest
from clan_emails.data_adapters.EmailsDataAdapter import EmailsDataAdapter
from clan_tools.utils.time import curr_utc_date_iso
import json

config = read_conf('config/logger.yml')
logging.config.dictConfig(config)
logger = logging.getLogger(__name__)


@timing
@click.command()
@click.option('--no_overlap', is_flag=True)
@click.option('--prod', is_flag=True)
def main(no_overlap, prod):
    yql_adapter = YQLAdapter()
    folder = "cloud_analytics" if prod else 'cloud_analytics_test'
    result_table_path = f'//home/{folder}/import/emails/nurturing_dashboard_dataset{"" if no_overlap else "_no_overlap"}'

    emails_programs = None
    if no_overlap:
        emails_programs = '''
        --sql
        (SELECT
                email,
                event,
                billing_account_id,
                program_name,
                stream_name,
                MIN_BY(event_time, event_time) as event_time,
                segment,
                MIN($date(CAST(event_time as String))) as first_add_to_nurture,
                MAX($date(CAST(event_time as String))) as last_add_to_nurture
            FROM `//home/cloud_analytics/cubes/emailing_events/emailing_events`
            WHERE
                event = 'add_to_nurture_stream' 
                and (event_time IS NOT NULL)  and  (event_time != '0000-00-00 00:00:00')
            GROUP BY
                email,
                billing_account_id,
                program_name,
                stream_name,
                segment,
                event
        )
        --endsql
        '''
    else:
        emails_programs = '''
        --sql
        (SELECT
                email,
                event,
                MIN_BY(billing_account_id, event_time) as billing_account_id,
                MIN_BY(program_name, event_time) as program_name,
                MIN_BY(stream_name, event_time) as stream_name,
                MIN_BY(event_time, event_time) as event_time,
                MIN_BY(segment, event_time) as segment,
                MIN($date(CAST(event_time as String))) as first_add_to_nurture,
                MAX($date(CAST(event_time as String))) as last_add_to_nurture

            FROM `//home/cloud_analytics/cubes/emailing_events/emailing_events`
            WHERE
                event = 'add_to_nurture_stream' 
                and (event_time IS NOT NULL)  and  (event_time != '0000-00-00 00:00:00')
            GROUP BY
                email, event)
        --endsql
        '''
    req: YqlSqlOperationRequest = yql_adapter.execute_query(dedent(f'''
        PRAGMA yt.Pool='cloud_analytics_pool';
            

        $date = ($str) -> {{RETURN CAST(SUBSTRING($str, 0, 10) AS DATE)}};
        $result_table = "{result_table_path}"; 

        --sql
        $cons_daily = (
            SELECT  billing_account_id,
                $date(event_time) as consumption_date,
                SUM(trial_consumption) as trial,
                SUM(real_consumption) as paid,
                ba_person_type,
                service_name,
                subservice_name,
                board_segment,
                sku_name,
                if(sku_lazy = 0, 'No', 'Yes') as sku_lazy
            FROM
                `//home/cloud_analytics/cubes/acquisition_cube/cube`
            WHERE
                nvl(event, '') in ('day_use', 'cloud_created', 'ba_created')
            GROUP BY
                billing_account_id,
                event_time,
                ba_person_type,
                service_name,
                subservice_name,
                board_segment,
                sku_name,
                sku_lazy
        );
       

        --sql
        $nurture = (
            SELECT
                email,
                billing_account_id,
                program_name,
                NVL(segment, 'Mass') as segment,
                IF(String::ToLower(stream_name) LIKE '%control%', 'control', 'test') as stream_name,
                $date(CAST(event_time as String)) as event_time,
                first_add_to_nurture,
                last_add_to_nurture
            FROM
                {emails_programs}
        );
        
        --sql
        $days_after = (
            SELECT
                nurture.*,
                cons_daily.*,
                (nurture.first_add_to_nurture IS NOT NULL) 
                    AND (cons_daily.consumption_date IS NOT NULL) 
                    AND (nurture.first_add_to_nurture <= cons_daily.consumption_date) AS is_consumption_after,

                DateTime::ToDays(cons_daily.consumption_date - nurture.first_add_to_nurture) AS days_after_first_add_to_nurture,
                DateTime::ToDays(cons_daily.consumption_date - nurture.last_add_to_nurture)  AS days_after_last_add_to_nurture
                
            FROM $nurture AS nurture  
            LEFT JOIN $cons_daily AS cons_daily 
            ON nurture.billing_account_id  = cons_daily.billing_account_id
        );

        --sql
        INSERT INTO $result_table WITH TRUNCATE
        SELECT days_after.*, 
               IF(Math::Round(days_after_first_add_to_nurture) > 0, 
                     CAST(Math::Round(days_after_first_add_to_nurture/30) AS String), 
                    'before_mailing') AS months_after_first_add_to_nurture,
               Math::Round(days_after_last_add_to_nurture/30)  AS months_after_last_add_to_nurture
        FROM $days_after AS days_after;


    '''),  to_pandas=False)
    req.run()
    query_res = req.get_results()
    logger.info(f'Request was sent.  Operation {req.operation_id}.')
    with open('output.json', 'w') as f:
        json.dump({"operation_id": req.share_url,
                    "table_path" : result_table_path,
                    'query_res' : str(query_res)
        }, f)
    


if __name__ == "__main__":
    main()