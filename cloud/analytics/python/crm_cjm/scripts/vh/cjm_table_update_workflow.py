import vh
import logging.config
from clan_tools.logging.logger import default_log_config
from clan_tools.utils.timing import timing
from clan_tools.vh.workflow import get_package
from clan_tools.vh.operations import spark_op
from clan_tools.vh.workflow import prepare_dependencies
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

    package = get_package(package_path='python/crm_cjm', local_script=local_src, files_to_copy=['src/', 'scripts/'])
    deps_yt_path = '//home/cloud_analytics/data_swamp/projects/crm_cjm/deps'
    driver_script = 'cjm_table_update.py'
    deps = prepare_dependencies(package=package, deps_dir=deps_yt_path, drivers=[driver_script])

    with vh.wait_for(*deps):
        spark_op(_name=f"CRM CJM Update ({'prod' if is_prod else 'test'})",
                 spyt_deps_dir=deps_yt_path,
                 spyt_secret='robot-clan-pii-yt-yt_token',
                 spyt_yt_token='robot-clan-pii-yt-yt_token',
                 retries_on_job_failure=0,
                 cluster='adhoc',
                 spyt_driver_filename=driver_script,
                 spyt_driver_args=['--is_prod' if is_prod else ''])

    workflow_id = '0b53accb-6cc4-4421-ade1-35c3d2259613' if is_prod else '141bcd13-9449-4ee4-b0ef-5ad1d7a756da'
    vh.run(wait=False,
           keep_going=True,
           start=with_start,
           workflow_guid=workflow_id,
           quota='coud-analytics',
           label=f'CRM CJM Update ({"prod" if is_prod else "test"})',
           backend=vh.NirvanaBackend())


if __name__ == '__main__':
    main()
