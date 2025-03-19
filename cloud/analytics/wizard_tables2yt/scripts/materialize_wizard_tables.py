import json
import logging.config
import time

import click
from wizard_tables2yt.datalens.charts import wizard_table_to_yt
from clan_tools.data_adapters.YTAdapter import YTAdapter
from clan_tools.data_adapters.ClickHouseYTAdapter import ClickHouseYTAdapter

from clan_tools.logging.logger import default_log_config

logging.config.dictConfig(default_log_config)
logger = logging.getLogger(__name__)


@click.command()
@click.option('--yt_token')
@click.option('--wizard_tables_path')
def main(yt_token, wizard_tables_path):
    yt_adapter = YTAdapter(token=yt_token)
    chyt_adapter = ClickHouseYTAdapter(yt_token)
    wizard_tables = chyt_adapter.execute_query(f'''
        select * from "{wizard_tables_path}"
        where is_regular=1
    ''', to_pandas=True)
    wizard_tables['is_regular'] = wizard_tables['is_regular'].astype(int)
    tables = [wizard_tables_path]
    for _, row in wizard_tables.iterrows():
        wizard_table_to_yt(row['datalens_url'], row['yt_path'], yt_adapter)
        curr_time = time.time()
        row['updated_at'] = curr_time

    tables.extend(wizard_tables.yt_path.values.tolist())

    yt_adapter.save_result(
        wizard_tables_path,
        schema={
            'created_at': float,
            'datalens_url': str,
            'yt_path': str,
            'is_regular': int
        },
        df=wizard_tables, append=False
    )
    with open('output.json', 'w') as f:
        json.dump({"tables_path": tables}, f)


if __name__ == '__main__':
    main()
