import json
import logging.config
from textwrap import dedent
import click

from clan_tools.data_adapters.ClickHouseYTAdapter import ClickHouseYTAdapter
from clan_tools.utils.conf import read_conf
from clan_tools.utils.timing import timing

config = read_conf('config/logger.yml')
logging.config.dictConfig(config)
logger = logging.getLogger(__name__)

@timing
@click.command()
@click.option('--write', is_flag=True)
def main(write):
    ch_adapter = ClickHouseYTAdapter()


    support_tickets_flat = dedent('''--sql
         SELECT assumeNotNull(billing_account_id) as billing_account_id,
                assumeNotNull(cloud_id) as cloud_id, 
                ba_person_type, segment, ba_state, ba_usage_status, key, created_time, first_paid
        FROM
            (SELECT YPathString(customFields, '/cloudId') as cloudId,
                       YPathString(customFields, '/billingId') as billingId,
                       toDateTime(created/1000) as created_time,
                       key
            FROM "//home/cloud_analytics/import/startrek/support/issues"
            WHERE cloudId != ''
            ) sup_cloud
                            
            LEFT JOIN
            
            (SELECT billing_account_id, cloud_id, ba_person_type, segment, ba_state, ba_usage_status,
                    toDateTime(first_first_paid_consumption_datetime) as first_paid
                    FROM "//home/cloud_analytics/cubes/acquisition_cube/cube"
                    WHERE event = 'cloud_created' 
                            AND ba_usage_status != 'service') as acq_cloud
            ON sup_cloud.cloudId = acq_cloud.cloud_id 
            --and toDate(acq_cloud.event_time) = toDate(sup_cloud.created_time)
            
        UNION ALL
        
        SELECT  billing_account_id, cloud_id, ba_person_type, segment, ba_state, ba_usage_status,  key, created_time, first_paid
        FROM
            (SELECT YPathString(customFields, '/cloudId') as cloudId,
                       YPathString(customFields, '/billingId') as billingId,
                       toDateTime(created/1000) as created_time,
                       key
            FROM "//home/cloud_analytics/import/startrek/support/issues"
            WHERE billingId != '' and cloudId = ''
            ) sup_ba
                            
            LEFT JOIN
            
            (SELECT billing_account_id, cloud_id, ba_person_type, segment,
                    ba_state, ba_usage_status,
                    toDateTime(first_first_paid_consumption_datetime)  as first_paid
                    FROM "//home/cloud_analytics/cubes/acquisition_cube/cube"
                    WHERE event = 'ba_created' 
                            AND ba_usage_status != 'service'
                        --    AND toDate(acq_ba.event_time) = toDate(sup_ba.created_time)
                            ) as acq_ba
            ON sup_ba.billingId = acq_ba.billing_account_id 
            
        UNION ALL
        
        SELECT 
               NULL as billing_account_id,
               NULL as cloud_id,
               NULL AS ba_person_type, 
               NULL AS segment,
               NULL AS ba_state,
               NULL AS ba_usage_status,
               key,
               toDateTime(created/1000) as created_time,
               NULL as first_paid
        FROM "//home/cloud_analytics/import/startrek/support/issues"
        WHERE YPathString(customFields, '/billingId') = '' and YPathString(customFields, '/cloudId') = ''
        --endsql
    ''')

    tickets_with_support_type = dedent(f'''--sql
        SELECT billing_account_id, 
               cloud_id, 
               ba_person_type, 
               segment, 
               ba_state,
               ba_usage_status,
               key,
               created_time,
               support_pay_type,
               if(isNull(first_paid) or (first_paid = '0000-00-00 00:00:00') 
                or (created_time < first_paid), 'trial', 'paid') as ba_pay_type
        FROM ( {support_tickets_flat} ) tickets
        LEFT JOIN 
            (SELECT DISTINCT task as st_key, pay_type as support_pay_type
            FROM "//home/cloud_analytics/import/tracker/support_tasks") as sup_type 
        ON sup_type.st_key = tickets.key
    --endsql
    ''')

    tickets_count = dedent(f'''--sql
        SELECT billing_account_id,
           cloud_id, 
           ba_person_type,
           toDate(toStartOfMonth(created_time)) as month, 
           support_pay_type, 
           count(distinct key) as n_tickets,
           ba_pay_type,
           segment,
           ba_state,
           ba_usage_status
        FROM ( {tickets_with_support_type} ) support
        GROUP BY  billing_account_id, cloud_id, month, support_pay_type, ba_person_type, ba_pay_type, segment,  ba_state,
           ba_usage_status
    --endsql
    ''')


    cons_without_support = dedent(f'''--sql
        SELECT  ifNull(billing_account_id, 'unknown') as billing_account_id,
                ifNull(cloud_id, 'unknown') as cloud_id, 
                ba_person_type, 
                segment, 
                if(isNull(first_first_paid_consumption_datetime) or
                     (toDateTime(event_time) < 
                         toDateTime(first_first_paid_consumption_datetime)) , 'trial', 'paid') as ba_pay_type,
                toStartOfMonth(toDate(event_time)) as month,
                ba_state,
                ba_usage_status,
                sum(real_consumption) as real_comsumption, 
                sum(trial_consumption) as trial_consumption
        FROM "//home/cloud_analytics/cubes/acquisition_cube/cube"
        WHERE event = 'day_use' AND ba_usage_status != 'service'
        GROUP BY billing_account_id,
                 cloud_id, 
                 ba_person_type, 
                 ba_pay_type,
                 segment, 
                 ba_state,
                 ba_usage_status,
                 month
              --endsql
        ''') 
   
    
    tickets_count_with_consumption = dedent(f'''--sql
        SELECT billing_account_id,
               cloud_id, 
               ba_person_type,
               month, 
               support_pay_type, 
               n_tickets,
               ba_pay_type,
               segment,
               ba_state,
               ba_usage_status,
               real_comsumption,
               trial_consumption
        FROM ({tickets_count}) tickets_count
        LEFT JOIN 
            (SELECT cloud_id, 
                    toStartOfMonth(toDate(event_time)) as month,
                    sum(real_consumption) as real_comsumption, 
                    sum(trial_consumption) as trial_consumption
            FROM "//home/cloud_analytics/cubes/acquisition_cube/cube"
            WHERE event = 'day_use' AND ba_usage_status != 'service'
            GROUP BY cloud_id, month) as cons
        ON tickets_count.cloud_id = cons.cloud_id 
                AND tickets_count.month = cons.month 
        --endsql
        ''')

    all_with_consumption = dedent(f'''--sql
        SELECT billing_account_id,
                cloud_id, 
                ba_person_type,
                month, 
                support_pay_type, 
                n_tickets,
                ba_pay_type,
                segment,
                ba_state,
                ba_usage_status,
                real_comsumption,
                trial_consumption
        FROM ({tickets_count_with_consumption})
        UNION ALL
        SELECT billing_account_id,
                cloud_id, 
                ba_person_type,
                month, 
                'unknown' as support_pay_type, 
                0 as n_tickets,
                ba_pay_type,
                segment,
                ba_state,
                ba_usage_status,
                real_comsumption,
                trial_consumption
        FROM ({cons_without_support})  as cws
        LEFT JOIN (SELECT billing_account_id as billingId, 
                          cloud_id as cloudId,
                          created_time
                  FROM  ({support_tickets_flat})
                  WHERE isNotNull(key) and ((billingId!='') or (cloudId!=''))) as tick
        ON cws.billing_account_id = tick.billingId
             AND cws.cloud_id = tick.cloudId  
             AND cws.month = toStartOfMonth(tick.created_time)
        WHERE isNull(tick.billingId) and isNull(tick.cloudId)
             or ((tick.billingId ='') and (tick.cloudId ='')
             AND billing_account_id != '' and cloud_id !='')
          
         --endsql
        ''')
 

    result_table = "support_tickets_count" if write else 'test_support_tickets'
    result_table_path = f'//home/cloud_analytics/import/tracker/{result_table}'

    # ch_adapter.execute_query(all_with_consumption, to_pandas=False) 
    # ch_adapter.insert_into(f'//home/cloud_analytics/import/tracker/cons', cons, append=False)

    ch_adapter.insert_into(result_table_path, all_with_consumption, append=False)
   



    with open('output.json', 'w') as f:
        json.dump({"table_path" : result_table_path }, f)
    


if __name__ == "__main__":
    main()