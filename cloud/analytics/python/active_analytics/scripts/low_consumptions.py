
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

config = read_conf('config/logger.yml')
logging.config.dictConfig(config)




@click.command()
@click.option('--ch_token')
@click.option('--tracker_token')
@click.option('--write', is_flag=True)
def main(ch_token, tracker_token, write):
        CLUSTER = 'hahn'
        ALIAS  = "*ch_public"
        TIMEOUT = 600
        START_DAYS=14 
        MIDDLE_DAYS=7
        END_DAYS=0
        CONS_PCT_THRESHOLD=15
        QUEUE="VAREVENTS"



        chyt_adapter = ClickHouseYTAdapter(ch_token, cluster=CLUSTER, alias=ALIAS,  timeout=TIMEOUT)
        last_days_cons_adapter = LastDaysConsumptionAdapter(chyt_adapter, start_days=START_DAYS, middle_days=MIDDLE_DAYS,
        end_days=END_DAYS)
        tracker_adapter = TrackerResultAdapter(token=tracker_token)

        cons_df: ComparedConsumption = last_days_cons_adapter.get_compared_consumption()
        pct_diff = (cons_df.last_period_cons/cons_df.period_before_last_cons).astype(float).values*100
        cons_df['pct_diff'] = np.round(100 - pct_diff, 2)
        low_cons = cons_df[cons_df.pct_diff > CONS_PCT_THRESHOLD].copy()
        tracker_ticket = low_cons.to_ticket(queue=QUEUE)
        dict_ticket = asdict(tracker_ticket)
        if write:
                tracker_adapter.send_data(dict_ticket)
      

        with open('output.json', 'w') as f:
                json.dump(dict_ticket, f)



if __name__ == '__main__':
    main()











