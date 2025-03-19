import vh
import logging.config
from clan_tools.logging.logger import default_log_config
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
    run_small_job_op(_name='CRM Leads history', max_ram=128, input=package, yql_token='robot-clan-pii-yt-yql_token',
                     script='scripts/run_yql_joblayer.py', script_args=['--path src/upsell/yql/crm_leads_history.sql'])

    workflow_id = '852f3ceb-cd20-41ce-b97e-ba291f78359a' if is_prod else '15bf5194-d74c-464c-b137-4f1800058bfc'
    vh.run(wait=False,
           keep_going=True,
           start=with_start,
           workflow_guid=workflow_id,
           project='upsell',
           quota='coud-analytics',
           label=f'CRM Leads history ({"prod" if is_prod else "test"})',
           backend=vh.NirvanaBackend())


if __name__ == '__main__':
    main()
