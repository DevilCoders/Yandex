import json
import click
import logging.config
from clan_tools.data_adapters.YQLAdapter import YQLAdapter
from clan_tools.logging.logger import default_log_config
from clan_tools.utils.timing import timing

logging.config.dictConfig(default_log_config)
logger = logging.getLogger(__name__)


@timing
@click.command()
@click.option('--path')
def run_yql_file(path: str) -> None:

    with open(path, 'r') as fin:
        yql_query_text = fin.read()

    yql_adapter = YQLAdapter()
    query = yql_adapter.execute_query(yql_query_text)
    query.run()
    query.get_results()
    is_success = YQLAdapter.is_success(query)

    if is_success:
        with open('output.json', 'w') as fout:
            json.dump({"yql_script_path" : path}, fout)


if __name__ == "__main__":
    run_yql_file()
