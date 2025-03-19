consumption_by_period = ''' 
const consumption_by_period = /*sql*/`
--sql
WITH   ['${params.billing_account_id.join('\\', \\'')}'] as ma
SELECT
        toDateTime(toStartOf${params.time_period}(toDateTime(event_time)))  time,
        master_account_id,
        billing_account_id,
        (SUM(real_consumption) + SUM(trial_consumption)) total_consumption
FROM "//home/cloud_analytics/cubes/acquisition_cube/cube"
WHERE event='day_use' AND  ba_usage_status != 'service'
        AND (toDateTime(event_time) >= toDateTime(toInt32(${moment(from).valueOf()/1000}))) 
            AND  (toDateTime(event_time) < toDateTime(toInt32(${moment(to).valueOf()/1000})))
        AND ((toDateTime(event_time) <= toDateTime(first_first_paid_consumption_datetime) 
            AND has(['${params.consumption_type.join('\\', \\'')}'], 'trial'))
                OR (toDateTime(event_time) >= toDateTime(first_first_paid_consumption_datetime)
                 AND has(['${params.consumption_type.join('\\', \\'')}'], 'paid')))
        AND (has(ma, 'All') OR has(ma, master_account_id) )
        AND (has(['${params.service_name.join('\\', \\'')}'], 'All') OR has(['${params.service_name.join('\\', \\'')}'], service_name) )
        AND isNotNull(master_account_id) AND master_account_id !=''
        AND segment = 'VAR'
GROUP BY time, master_account_id, billing_account_id
HAVING total_consumption  > ${params.consumption_threshold}
--endsql
`;

'''


sub_query = '''
const sub_query = /*sql*/`
SELECT time, master_account_id as billing_account_id, account_name as name, sub_consumption, sub_count
FROM (
      SELECT time,
             master_account_id, 
             COUNT(DISTINCT billing_account_id) sub_count,
             sum(total_consumption) as sub_consumption
      
      FROM (
        ${consumption_by_period}
      )
      GROUP BY time, master_account_id
      HAVING sub_count >= ${params.min_n_subaccounts} AND sub_count <= ${params.max_n_subaccounts} )  as a

       LEFT JOIN 
      
   
        (SELECT DISTINCT billing_account_id AS master_account_id,  
                account_name
         FROM "//home/cloud_analytics/cubes/acquisition_cube/cube"
         WHERE event='ba_created' AND  ba_usage_status != 'service'
         ) as b
         
    USING master_account_id
ORDER BY  time
`;
'''


by_accounts = '''
const by_accounts = /*sql*/`
select * from 
(${sub_query}) as cons
inner join
(select billing_account_id, min(toDate(substring(date_qualified, 1, 10))) as converted_date
from "//home/cloud_analytics/lunin-dv/dashboard_tables/var_info"
WHERE length(date_qualified) >= 10
GROUP BY billing_account_id) as crm
on cons.billing_account_id = crm.billing_account_id
WHERE (converted_date >= toDateTime(toInt32(${from_conv_m}))) 
       AND  (converted_date <= toDateTime(toInt32(${to_conv_m})))
`;
'''


subaccounts_count_vs_total_cons = '''
const query = /*sql*/`
select time, 'total' as billing_account_id, 'total' as name, sum(sub_consumption) as total_sub_consumption,  toUInt32(sum(sub_count)) as total_sub_count
from 
(${by_accounts}) as sub_accs
group by time
order by time
`;
'''

accounts_count = '''
const query = `
select * from 
(${sub_query}) as cons
inner join
(select billing_account_id, min(toDate(substring(date_qualified, 1, 10))) as converted_date
from "//home/cloud_analytics/lunin-dv/dashboard_tables/var_info"
WHERE length(date_qualified) >= 10
GROUP BY billing_account_id) as crm
on cons.billing_account_id = crm.billing_account_id
WHERE (converted_date >= toDateTime(toInt32(${from_conv_m}))) 
       AND  (converted_date <= toDateTime(toInt32(${to_conv_m})))
`;
'''