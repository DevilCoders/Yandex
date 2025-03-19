from clan_tools.data_adapters.YQLAdapter import YQLAdapter
import logging.config
from clan_tools.utils.timing import timing
import click
from yql.client.operation import YqlSqlOperationRequest
import json
from clan_tools.logging.logger import default_log_config
from clan_tools.utils.path import childfile2str
from  scoring_of_potential import yql
logging.config.dictConfig(default_log_config)
logger = logging.getLogger(__name__)


@timing
@click.command()
@click.option('--edges_path')
@click.option('--result_table_path')
@click.option('--script',  default='neighbours.sql')
def main(edges_path, result_table_path, script):
    yql_adapter = YQLAdapter()
    query = (
        childfile2str(yql.__file__, script)
        .format(result_table=result_table_path,
                edges_path=edges_path)
    )

    req: YqlSqlOperationRequest = yql_adapter.execute_query(
        query, to_pandas=False)
    yql_adapter.yql_respone_request_to_json(req, result_table_path)


if __name__ == "__main__":
    main()
