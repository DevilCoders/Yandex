from clan_tools.data_adapters.YQLAdapter import YQLAdapter
import logging.config
from clan_tools.utils.timing import timing
import click
from yql.client.operation import YqlSqlOperationRequest
import json
from clan_tools.utils import yql as tools_yql
from clan_tools.logging.logger import default_log_config
from clan_tools.utils.path import childfile2str
from  scoring_of_potential import yql
logging.config.dictConfig(default_log_config)
logger = logging.getLogger(__name__)


@timing
@click.command()
@click.option('--script')
@click.option('--result_table_path', default='//home/cloud_analytics/scoring_of_potential/cloud_criterions')
def main(script, result_table_path):
    yql_adapter = YQLAdapter()
    query = (
        childfile2str(yql.__file__, script)
        .format(result_table=result_table_path)
    )

    req: YqlSqlOperationRequest = yql_adapter.execute_query(
        query, to_pandas=False)
    req = YQLAdapter.attach_files(tools_yql.__file__, 'tables.sql', req)

    req.run()
    req.get_results()

    with open('output.json', 'w') as f:
        json.dump({"operation_id": req.share_url,
                    "table_path": result_table_path
                }, f)


if __name__ == "__main__":
    main()
