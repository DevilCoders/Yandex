import pandas as pd
from io import BytesIO
from datetime import datetime
from clan_tools.utils.time import parse_date
from active_analytics.data_model.Consumption import TimePeriod, ComparedConsumption
from clan_tools.utils.timing import timing
from clan_tools.data_adapters.ClickHouseYTAdapter import  ClickHouseYTAdapter
import logging 
from textwrap import dedent
logger = logging.getLogger(__name__)



# TODO mean на min заменить


class DailyConsumptionAdapter:
    def __init__(self, chyt_adapter: ClickHouseYTAdapter):
        self._chyt_adapter  = chyt_adapter
       
    @timing
    def consumption(self, date:datetime, window_len=30):
        logger.info(f'Collecting consumption data for {date}')
        date_s = f"toDate('{date.isoformat()}')"
        interest_services = "'compute', 'cloud_ai', 'mdb', 'mk8s'"
        other_fields = dedent('''
        --sql
        groupArray(segment)[-1] AS segment,
        arrayFilter(x->x!='unmanaged', groupArray(sales_name))[-1] AS sales_name,
        groupArray(account_name)[-1] AS account_name
        --endsql''')
        small_window_len = 3
        week_window_len = 6
        cons_by_service = dedent(f'''
          --sql
          SELECT billing_account_id, 
                service_name,
                toDate(event_time) AS event_time,
                sum(real_consumption) AS paid_cons,
                sum(real_consumption + trial_consumption) AS total_cons,
                {other_fields}
          FROM "//home/cloud_analytics/cubes/acquisition_cube/cube"
          WHERE event = 'day_use' AND ba_usage_status != 'service' 
              AND toDate(event_time) >= ({date_s} - INTERVAL {window_len} DAY)
              AND toDate(event_time) <= {date_s}
              AND service_name != 'cloud_ai'
          GROUP BY billing_account_id, service_name, event_time
          ORDER BY event_time
          --endsql
        ''')

        small_window_cond = dedent(f'''((toDate(event_time) >= {date_s} - INTERVAL {small_window_len+1} DAY)
                                        AND (toDate(event_time) <= {date_s} - INTERVAL 1 DAY))''')
        week_window_cond = dedent(f'''((toDate(event_time) >= {date_s} - INTERVAL {week_window_len+1} DAY)
                                      AND (toDate(event_time) <= {date_s} - INTERVAL 1 DAY))
                                      AND service_name in ({interest_services})
                                      ''')

        last_half_cond = f'(toDate(event_time) >= {date_s} - INTERVAL {window_len//2} DAY)'
        prev_half_cond =  f'(toDate(event_time) <= {date_s} - INTERVAL {window_len//2} DAY)'
        calculated_columns = dedent(f'''
           --sql
            count(DISTINCT if(assumeNotNull({week_window_cond}), event_time, toDate('1970-01-01'))) AS count_days,
            sumIf(paid_cons, toDate(event_time)={date_s} - INTERVAL 1 DAY) AS paid_cons_1,
            sumIf(paid_cons, toDate(event_time)={date_s} - INTERVAL 7 DAY) AS paid_cons_7,
            sumIf(paid_cons, toDate(event_time)={date_s}) AS paid_cons_cur,
            sumIf(total_cons, toDate(event_time)={date_s}) AS total_cons_cur,
            minIf(paid_cons, {small_window_cond}) AS paid_cons_1_3,
            minIf(total_cons, {small_window_cond}) AS total_cons_1_3,
            minIf(paid_cons, {week_window_cond}) AS min_paid_cons,
            medianIf(paid_cons, {last_half_cond}) AS median_last_half,
            medianIf(paid_cons, {prev_half_cond}) AS median_prev_half,
            avgIf(paid_cons, {prev_half_cond}) AS avg_prev_half,
            (paid_cons_cur - paid_cons_1_3)/paid_cons_1_3 AS paid_diff,
            (total_cons_cur - total_cons_1_3)/total_cons_1_3 AS total_diff,
            {other_fields}
            --endsql''')

        filter_condition = dedent(f'''
        --sql 
            paid_diff <= -0.5 AND total_diff <= -0.5 
            AND paid_cons_1_3 > 1000 AND paid_cons_cur < min_paid_cons*0.85
            AND ((count_days = {week_window_len+2}) OR (isNull(paid_cons_cur) AND isNull(min_paid_cons)))
            OR (((median_prev_half>1000) AND (avg_prev_half > 1000) AND (paid_cons_1_3 > 1000) )  
                  AND ((isNotNull(median_prev_half) AND (median_last_half<median_prev_half*0.5))
                        OR (isNull(min_paid_cons))))
            AND isNotNull(paid_cons_7)  AND paid_cons_7>1000 AND paid_cons_cur < paid_cons_7*0.85
            AND NOT (isNull(paid_cons_cur) AND isNotNull(paid_cons_1))
        --endsql
        ''')
        last_cons_by_services = dedent(f'''
          --sql
          SELECT  billing_account_id, 
                  service_name,
                  {calculated_columns}
          FROM ({cons_by_service}) AS cons_by_service
          GROUP BY  billing_account_id, service_name
          HAVING {filter_condition}
          --endsql
        ''')

        cons_by_intrest_services = dedent(f'''--sql
          SELECT *
          FROM ({last_cons_by_services}) AS last_cons_by_services
          WHERE service_name IN ({interest_services})
        --endsql''')

        total_cons = dedent(f'''--sql
          SELECT billing_account_id, 
                 'total' AS service_name,
                 {calculated_columns}
          FROM ({cons_by_service}) AS cons_by_service
          GROUP BY  billing_account_id
          HAVING {filter_condition}
        --endsql''')

        union_cons = dedent(f'''--sql
          {cons_by_intrest_services}
          UNION ALL
          {total_cons}
        --endsql''')

       
        result_df = self._chyt_adapter.execute_query(union_cons, to_pandas=True)
        return result_df
    
  

    

    