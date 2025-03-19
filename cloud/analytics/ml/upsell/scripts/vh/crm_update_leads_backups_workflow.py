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

    package = get_package(package_path='ml/upsell', local_script=local_src, files_to_copy=['src/', 'scripts/'])
    run_small_job_op(_name='Backup update_leads table',
                     max_ram=128, input=package,
                     script='scripts/crm_update_leads_backups.py',
                     yql_token='robot-clanalytics-yql',
                     yt_token='robot-clanalytics-yt')

    workflow_id = 'e04c7dd4-b0f8-4506-9118-faaac2e1868b' if is_prod else '4f8ed00e-7e4d-4750-8d30-5d3f21400f86'
    vh.run(wait=False,
           keep_going=True,
           start=with_start,
           workflow_guid=workflow_id,
           project='upsell',
           quota='coud-analytics',
           label=f'Backuping table update_leads ({"prod" if is_prod else "test"})',
           backend=vh.NirvanaBackend())


if __name__ == '__main__':
    main()
