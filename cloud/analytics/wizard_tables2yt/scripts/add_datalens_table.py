import json
import logging.config
import time

import click
import pandas as pd
from wizard_tables2yt.datalens.charts import wizard_table_to_yt
from clan_tools.data_adapters.YTAdapter import YTAdapter
from clan_tools.logging.logger import default_log_config

logging.config.dictConfig(default_log_config)
logger = logging.getLogger(__name__)


@click.command()
@click.option('--datalens_url')
@click.option('--yt_path')
@click.option('--is_regular')
@click.option('--yt_token')
@click.option('--wizard_tables_path')
def main(datalens_url, wizard_tables_path, yt_path, is_regular, yt_token):
    is_regular_int = int(is_regular == 'True')
    yt_adapter = YTAdapter(token=yt_token)
    curr_time = time.time()
    result_df = pd.DataFrame([[curr_time, curr_time, str(datalens_url), str(yt_path), is_regular_int]],
                             columns=['created_at', 'updated_at', 'datalens_url', 'yt_path', 'is_regular'])
    wizard_table_to_yt(datalens_url, yt_path, yt_adapter)
    yt_adapter.save_result(wizard_tables_path,
                           schema={
                               'created_at': float,
                               'updated_at': float,
                               'datalens_url': str,
                               'yt_path': str,
                               'is_regular': int
                           },
                           df=result_df)
    with open('output.json', 'w') as f:
        json.dump({"table_path": yt_path}, f)


if __name__ == '__main__':
    main()
