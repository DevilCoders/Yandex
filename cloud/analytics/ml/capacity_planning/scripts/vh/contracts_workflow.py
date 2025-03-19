import vh
from clan_tools.logging.logger import default_log_config
import logging.config
from clan_tools.utils.timing import timing
from clan_tools.vh.workflow import get_package
from clan_tools.vh.operations import run_small_job_op
import click

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

    package = get_package(package_path='ml/capacity_planning', local_script=local_src, files_to_copy=['src/', 'scripts/'])

    run_small_job_op(_name='CRM contracts update', max_ram=128, input=package, yql_token='robot-clan-pii-yt-yql_token',
                     script='scripts/run_yql_joblayer.py', script_args=['--path src/capacity_planning/yql/contracts.sql'])

    workflow_id = '51c40f7b-c31d-4c3f-8d9c-67d42128c346' if is_prod else '4cc62acf-d3ca-4dbc-9f00-a092395bc916'
    vh.run(wait=False,
           keep_going=True,
           start=with_start,
           workflow_guid=workflow_id,
           project='capacity_planning',
           quota='coud-analytics',
           label=f'CRM contracts ({"prod" if is_prod else "test"})',
           backend=vh.NirvanaBackend())


if __name__ == '__main__':
    main()
