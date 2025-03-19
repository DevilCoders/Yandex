#!/usr/bin/env python
# coding: utf-8


import pathlib
import numpy as np
import json
import pandas as pd
from dataclasses import asdict
from clan_tools.data_adapters.TrackerAdapter import TrackerAdapter
from datetime import date
from autoabuse.preprocessing import get_ip_from_text
from autoabuse.send_messages import send_message, prepare_messages, write_label_history, suspend_trial
from autoabuse.traffic_stats import get_traffic_stats
from clan_tools.data_adapters.YTAdapter import YTAdapter
from clan_tools.logging.logger import default_log_config

import logging.config

logging.config.dictConfig(default_log_config)

logger = logging.getLogger(__name__)



def main():
    labels_history_path = '//home/cloud_analytics/import/network-logs/autoabuse/labeled_tickets'
    current_tickets_path = '//home/cloud_analytics/import/network-logs/autoabuse/current_tickets'
    traffic_stat_path = '//home/cloud_analytics/import/network-logs/autoabuse/ba_ips_ssh_rdp'
    to_suspend_table_path = '//home/cloud_analytics/antifraud_suspension/to_suspend'

    labeled_tickets_keys = []
    logger.debug('Collecting issues')
    issues = (TrackerAdapter()
                .get_user_issues(queue='CLOUDABUSE',
                                 status=None,
                                 components=[84312], #autoabuse component
                                 date_from_created=date.today().isoformat(),
                                 include_description=True))
    def try_asdict(issue):
        try:
            return asdict(issue) 
        except:
            pass
    
    records = []
    for issue in issues:
        record = try_asdict(issue)
        if record is not None:
            records.append(record)

    issues_df = pd.DataFrame.from_records(records)

    if issues_df.shape[0] == 0:
        return labeled_tickets_keys

    logger.debug('Extracting ips')
    ip_series = issues_df['description'].apply(get_ip_from_text)
    issues_df['ips'] = ip_series

    key_ip = issues_df[['key', 'ips']].explode('ips').dropna()
    key_ip.columns = ['key', 'ip']

    logger.debug('Saving ips to yt')
    YTAdapter().save_result(
        result_path=current_tickets_path,
        schema={'key':'string', 'ip':'string'},
        df=key_ip,
        append=False)


    logger.debug('Collecting traffic stats')
    ip_clouds = get_traffic_stats(traffic_stat_path, current_tickets_path, labels_history_path)

    
    if ip_clouds.shape[0] == 0:
        return labeled_tickets_keys

    def not_frauders(traffic):
        inv_traffic = 1/traffic
        median_traffic = inv_traffic.median()
        mad = np.abs(inv_traffic - median_traffic).median()
        outliers = (inv_traffic - median_traffic)/mad > 3
        return outliers

    small_traffic = not_frauders(ip_clouds['traffic'])

    keys_traffic = (ip_clouds[~small_traffic]
                    .explode('keys')
                    .sort_values(by='keys')).copy()

    keys_traffic['block'] = (
        (keys_traffic['ba_usage_status'] == 'trial') 
        & (keys_traffic['n_unique_ssh_rdp_dests'] > 1000)
        & (
                (keys_traffic['segment'] == 'Mass')
                | (keys_traffic['segment'] == '')
                | (keys_traffic['segment'].isna())

        )
    )

    logger.debug('Preparing comments to issues')

    keys_messages = prepare_messages(keys_traffic)

    logger.debug('Creating comments')

    labeled_tickets = (issues_df
        .merge(keys_messages, left_on='key', right_index=True, how='inner'))

    suspend_trial(to_suspend_table_path, keys_traffic)

    
    labeled_tickets.apply(send_message, axis=1)
    labeled_tickets_keys = labeled_tickets['key'].values.tolist()
    logger.debug(f'Commented {labeled_tickets_keys}')

    write_label_history(labeled_tickets, labels_history_path)



    logger.debug(f'Finished')   
    
if __name__ == "__main__":
    labeled_tickets_keys = main()
    with open('output.json', 'w') as f:
        json.dump({"handled_tickets" : labeled_tickets_keys}, f)




