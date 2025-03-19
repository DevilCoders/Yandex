import pandas as pd
from io import BytesIO
from clan_tools.utils.time import parse_date
from active_analytics.data_model.Consumption import TimePeriod, ComparedConsumption
from clan_tools.utils.timing import timing
from clan_tools.data_adapters.ClickHouseYTAdapter import  ClickHouseYTAdapter
from clan_tools.utils.csv import str_to_df

LAST_DAYS_CONS_SQL_STR = '''
--sql
SELECT master_account_id, account_name, period_before_last_cons, last_period_cons
FROM 

    (  
        WITH (toStartOfDay(now()) - INTERVAL {start_days} DAY) as period_start,
            (toStartOfDay(now()) - INTERVAL {middle_days} DAY) as period_middle,
            toStartOfDay(now() - INTERVAL {end_days} DAY) as period_end,
            10 as consumption_threshold
        SELECT  period_start,
                period_middle,
                period_end, 
                master_account_id,
                sumIf(real_consumption + trial_consumption, 
                    toDateTime(event_time) >= period_start
                    AND toDateTime(event_time) < period_middle) AS period_before_last_cons,
                sumIf(real_consumption+trial_consumption, 
                    toDateTime(event_time) >= period_middle
                    AND toDateTime(event_time) < period_end)  AS last_period_cons
        FROM "//home/cloud_analytics/cubes/acquisition_cube/cube"
        WHERE event='day_use' AND  ba_usage_status != 'service'
            AND (toDateTime(event_time) > period_start) AND  (toDateTime(event_time) < period_end)
            AND isNotNull(master_account_id)
        GROUP BY master_account_id
        HAVING period_before_last_cons  > consumption_threshold 
            AND  last_period_cons > consumption_threshold) AS cons
    
    LEFT JOIN 
    
        (SELECT billing_account_id,
                account_name
        FROM "//home/cloud_analytics/cubes/acquisition_cube/cube"
        WHERE event='ba_created' AND  ba_usage_status != 'service'
        ) AS names
    
    ON cons.master_account_id = names.billing_account_id
    

FORMAT CSVWithNames
--endsql
'''

LAST_PERIODS_SQL = '''
--sql
WITH toDate((toStartOfDay(now()) - INTERVAL {start_days} DAY)) as period_start,
     toDate((toStartOfDay(now()) - INTERVAL {middle_days} DAY)) as period_middle,
     toDate(toStartOfDay(now() - INTERVAL {end_days} DAY)) as period_end
SELECT period_start, period_middle, period_end
FORMAT CSVWithNames
--endsql
'''


class LastDaysConsumptionAdapter:
    def __init__(self, chyt_adapter: ClickHouseYTAdapter, 
                 start_days:int=14, middle_days:int=7, end_days:int=0):
        self._chyt_adapter  = chyt_adapter
        self._start_days = start_days
        self._middle_days = middle_days
        self._end_days = end_days

    @timing
    def get_consumption_df(self):
        last_days_str = LAST_DAYS_CONS_SQL_STR.format(start_days=self._start_days, 
            middle_days=self._middle_days, end_days=self._end_days)
        last_cons:str = self._chyt_adapter.execute_query(last_days_str)
        last_cons_df:pd.DataFrame = str_to_df(last_cons)
        return last_cons_df
    
    def get_last_periods(self):
        last_periods_sql_str =  LAST_PERIODS_SQL.format(start_days=self._start_days, 
            middle_days=self._middle_days, end_days=self._end_days)
        last_periods_str:str = self._chyt_adapter.execute_query(last_periods_sql_str)

        last_cons_df:pd.DataFrame = str_to_df(last_periods_str,
                                 parse_dates=['period_start', 'period_middle', 'period_end'], 
                                 date_parser=parse_date)
        periods_dict = last_cons_df.iloc[0, :].to_dict()
        return periods_dict


    
    def get_compared_consumption(self):
        cons_df: pd.DataFrame = self.get_consumption_df()
        periods_dict = self.get_last_periods()
        compared_cons = ComparedConsumption(cons_df.values, columns=cons_df.columns,
            period_before_last=TimePeriod(periods_dict['period_start'], periods_dict['period_middle']),
            last_period=TimePeriod(periods_dict['period_middle'], periods_dict['period_end'])
        )
        return compared_cons
        

    