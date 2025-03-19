import vh
import click
from clan_tools.logging.logger import default_log_config
import logging.config
from clan_tools.utils.timing import timing
from clan_tools.vh.operations import run_yql_script

logging.config.dictConfig(default_log_config)
logger = logging.getLogger(__name__)


@timing
@click.command()
@click.option('--local_src', is_flag=True, default=False)
@click.option('--is_prod', is_flag=True, default=False)
@click.option('--with_start', is_flag=True, default=False)
def main(local_src,  is_prod, with_start):
    logger.debug('Starting graph')
    logger.info('Updating production workflow' if is_prod else 'Testing workflow')
    logger.info('Workflow will start' if with_start else 'Workflow will not start')

    baid_puid = run_yql_script("src/ml_model_features/yql/dicts/baid_puid.sql")
    with vh.wait_for(baid_puid):
        run_yql_script("src/ml_model_features/yql/dicts/yandexuid_puid.sql")

    workflow_id = 'fcddbea8-177f-4a5e-b8e4-a4327d672620' if is_prod else '1b2cc312-2518-4b68-b1e3-94433b4229d2'
    vh.run(wait=False,
           keep_going=True,
           start=with_start,
           workflow_guid=workflow_id,
           quota='coud-analytics',
           backend=vh.NirvanaBackend())


if __name__ == '__main__':
    main()
