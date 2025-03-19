
import logging.config
from active_analytics.data_adapter.LastConsumptionAdapter import LastDaysConsumptionAdapter
import pandas as pd
from clan_tools.data_adapters.ClickHouseYTAdapter import ClickHouseYTAdapter
from clan_tools.data_adapters.TrackerResultAdapter import TrackerResultAdapter
import clan_tools
import click
from active_analytics.data_model.Consumption import ComparedConsumption
import requests
import json
from clan_tools.utils.conf import read_conf
import numpy as np
from dataclasses import asdict
from active_analytics.data_adapter.DailyConsumptionAdapter import DailyConsumptionAdapter
from clan_tools.utils.token import get_token
from typing import Dict
from clan_tools.data_adapters.TrackerResultAdapter import TrackerResultAdapter
from clan_tools.data_model.tracker.TrackerTicket import TrackerTicket
from dataclasses import dataclass, asdict
from datetime import datetime, timedelta
from clan_tools.utils.time import rus_format_date
from active_analytics.notifiers.DailyConsumptionNotifier import DailyConsumptionNotifier
from active_analytics.data_adapter.DailyConsumptionLogAdapter import DailyConsumptionLogAdapter
from clan_tools.data_adapters.YTAdapter import YTAdapter

logger_config = read_conf('config/logger.yml')
config = read_conf('config/daily_alert_config_test.yml')
logging.config.dictConfig(logger_config)

logger = logging.getLogger(__name__)


@click.command()
@click.option('--ch_token')
@click.option('--tracker_token')
@click.option('--write', is_flag=True)
def main(ch_token, tracker_token, write):
    chyt_adapter = ClickHouseYTAdapter(ch_token)
    daily_cons_adapter = DailyConsumptionAdapter(chyt_adapter)
    yt_adapter = YTAdapter(ch_token)

    default_responsibles : Dict[str, str] = config['default']['segment']['responsibles']
    default_followers : Dict[str, str] = config['default']['service']['followers']

    if write:
        daily_cons_log_adapter = DailyConsumptionLogAdapter(chyt_adapter, yt_adapter, 
                                result_path='//home/cloud_analytics/alerts/logs/daily_cons_tracker')
        tracker_adapter = TrackerResultAdapter(tracker_token)
    else:
        daily_cons_log_adapter = None
        tracker_adapter = None
       


    notifier = DailyConsumptionNotifier(daily_cons_adapter, tracker_adapter, 
                                        default_responsibles, default_followers, queue='CLANALERTS',
                                        log_adapter=daily_cons_log_adapter)

    cons_df:pd.DataFrame = notifier.notify(date = datetime.now().date() - timedelta(days=2))

    with open('output.json', 'w') as f:
            json.dump(cons_df.to_json(), f)

    # notifier.notify_over_period(datetime.now().date() - timedelta(days=1), num_days_before=20)

if __name__ == "__main__":
    main()