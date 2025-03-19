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
@click.option('--neighbours_path')
@click.option('--result_table_path')
def main(neighbours_path, result_table_path):
    yql_adapter = YQLAdapter()
    query = (
        childfile2str(yql.__file__, 'passport_id2spark_id.sql')
        .format(result_table_path=result_table_path,
                neighbours_path=neighbours_path)
    )

    req: YqlSqlOperationRequest = yql_adapter.execute_query(
        query, to_pandas=False)
    yql_adapter.yql_respone_request_to_json(req, result_table_path)


if __name__ == "__main__":
    main()
