from clan_tools.data_adapters.YTAdapter import YTAdapter
import pandas as pd
from datetime import datetime, timedelta
from clan_tools.data_adapters.ClickHouseYTAdapter import ClickHouseYTAdapter
from textwrap import dedent
from clan_tools.utils.time import curr_utc_date_iso
from clan_tools.utils.timing import timing
timing

class DailyConsumptionLogAdapter:
    def __init__(self, chyt_adapter:ClickHouseYTAdapter, yt_adapter:YTAdapter, result_path:str):
        self._yt_adapter = yt_adapter
        self._result_path = result_path
        self._chyt_adapter = chyt_adapter

    @timing
    def log_data(self, df:pd.DataFrame):
        yt_schema = [
                {"name": "billing_account_id", "type": "string"},
                {"name": "log_utc_time", "type": "datetime"}
            ]
        log_df = df.copy()
        log_df['log_utc_time'] = YTAdapter.datetime2ytdatetime(datetime.utcnow())
        self._yt_adapter.save_result(self._result_path, yt_schema, log_df, append=True)
    
    @timing
    def get_last_logged_ba(self, num_last_days:int=30):
        res:pd.DataFrame = None
        if self._yt_adapter.yt.exists(self._result_path):
            query = dedent(f'''--sql
                SELECT DISTINCT billing_account_id
                FROM "{self._result_path}"
                WHERE log_utc_time > toDate('{curr_utc_date_iso(days_lag = num_last_days) }')
            --endsql
            ''')
            res = self._chyt_adapter.execute_query(query, to_pandas=True)
        return res

