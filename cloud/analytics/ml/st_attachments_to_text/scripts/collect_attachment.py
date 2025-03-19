import os
import sys
import re
sys.path.append(os.path.abspath(f"{os.environ['SOURCE_CODE_PATH']}/src"))
import pkg_resources
import calendar
import numpy as np
import pandas as pd
import logging.config
from datetime import datetime, date, timedelta
import nirvana_dl
import pip
import clan_tools
from clan_tools.logging.logger import default_log_config
from clan_tools.data_adapters.YTAdapter import YTAdapter
from clan_tools.data_adapters.YQLAdapter import YQLAdapter
from clan_tools.data_adapters.TrackerAdapter import TrackerAdapter
from clan_tools.secrets.Vault import Vault

logging.config.dictConfig(default_log_config)
logger = logging.getLogger(__name__)

Vault(token=nirvana_dl.get_options()['user_requested_secret']).get_secrets()
yt_adapter = YTAdapter(token=os.environ['YT_TOKEN'])
tracker_adapter = TrackerAdapter()
output_path = os.environ['DATA_PATH']


# Vault().get_secrets()
# yt_adapter = YTAdapter()
# tracker_adapter = TrackerAdapter()
# output_path = './'


def main():
    # last_date = yt_adapter.read_table('//home/cloud_analytics/ml/st_attachment_to_text/last_date', to_pandas=True)['date'][0]
    # last_date = datetime.fromtimestamp(int(last_date)).strftime('%Y-%m-%d %H:%M:%S')
    # new_last_date = calendar.timegm(datetime.utcnow().utctimetuple())
    # df = pd.DataFrame([new_last_date], columns=['date'])
    # schema = [{'name': 'date', 'type': 'int64'}]
    # yt_adapter.save_result('//home/cloud_analytics/ml/st_attachment_to_text/last_date', schema, df, append=False)
    df = yt_adapter.read_table('//home/cloud_analytics/ml/st_attachment_to_text/CLOUDSUPPORT', to_pandas=True)
    processed_tickets = df['st_key'].unique()
    logger.debug(processed_tickets)
    last_date = str(datetime.now() - timedelta(1))[:10]
    # last_date = "2022-07-01"
    logger.debug(last_date)
    issues = tracker_adapter.st_client.issues.find(filter={
        'queue': 'CLOUDSUPPORT',
        'created': {'from': last_date}
    })

    counter = 0
    for issue in issues:
        logger.debug(f"SPOTED: {issue.key}")
        if issue.key not in processed_tickets:
            logger.debug(f"INCLUDED: {issue.key}")
            # if issue.createdAt >= last_date: # str(datetime.strptime(issue.createdAt[:19], '%Y-%m-%dT%H:%M:%S')) >= last_date: #
            for attach in issue.attachments.get_all():
                if attach.mimetype in ['application/pdf', 'image/jpeg', 'image/png']:
                    counter += 1
                    filetype = attach.mimetype[attach.mimetype.find('/')+1:]
                    attach.download_to(output_path)
                    os.rename(output_path + f'/{attach.name}', output_path + f'/{filetype}_{issue.key}_{attach.id}.{filetype}')
                    logger.debug(output_path + f'/{filetype}_{issue.key}_{attach.id}.{filetype}')
    assert(counter > 0)
if __name__ == '__main__':
    main()