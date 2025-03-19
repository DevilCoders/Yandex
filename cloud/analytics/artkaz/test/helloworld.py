import json
import click
import os
from clan_tools.data_adapters.ClickHouseYTAdapter import ClickHouseYTAdapter


def hello():
    chyt = ClickHouseYTAdapter(token=os.environ['YT_TOKEN'], alias="*ch_public")
    with open('output.json', 'w') as f:
        json.dump({

            "now": chyt.execute_query("""SELECT now()"""),
        }, f)

if __name__ == '__main__':
    hello()