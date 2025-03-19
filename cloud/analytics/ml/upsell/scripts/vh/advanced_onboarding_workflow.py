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
    run_small_job_op(_name='Advanced onboarding',
                     max_ram=128, input=package,
                     script='scripts/advanced_onboarding_daily.py',
                     yql_token='robot-clan-pii-yt-yql_token',
                     yt_token='robot-clan-pii-yt-yt_token')

    workflow_id = '4e0bfee1-b478-4fed-8587-c5fe3c0378b9' if is_prod else '4f8ed00e-7e4d-4750-8d30-5d3f21400f86'
    vh.run(wait=False,
           keep_going=True,
           start=with_start,
           workflow_guid=workflow_id,
           project='upsell',
           quota='coud-analytics',
           label=f'Advanced Onboarding ({"prod" if is_prod else "test"})',
           backend=vh.NirvanaBackend())


if __name__ == '__main__':
    main()
