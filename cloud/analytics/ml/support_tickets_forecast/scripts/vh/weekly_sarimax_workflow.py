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
@click.option('--rebuild', is_flag=True, default=False)
@click.option('--with_start', is_flag=True, default=False)
def main(local_src: bool = False,  is_prod: bool = False, rebuild: bool = False, with_start: bool = False) -> None:
    logger.debug('Starting graph')
    logger.info('Updating production workflow' if is_prod and not rebuild else 'Testing workflow')
    logger.info('Rebuilding results' if rebuild else 'Appending only new results')
    logger.info('Workflow will start' if with_start else 'Workflow will not start')

    package = get_package(package_path='ml/support_tickets_forecast', local_script=local_src, files_to_copy=['src/', 'scripts/'])
    deps_yt_path = '//home/cloud_analytics/ml/support_tickets_forecast/deps'
    driver_script = 'weekly_sarimax.py'
    deps = prepare_dependencies(package=package, deps_dir=deps_yt_path, drivers=[driver_script])

    with vh.wait_for(*deps):
        spyt_args = []
        if is_prod:
            spyt_args.append('--is_prod')
        if rebuild:
            spyt_args.append('--rebuild')
        spark_op(_name=f"Sarimax ({'prod' if is_prod else 'test'})",
                 spyt_deps_dir=deps_yt_path,
                 spyt_secret='robot-clan-pii-yt-yt_token',
                 spyt_yt_token='robot-clan-pii-yt-yt_token',
                 retries_on_job_failure=0,
                 cluster='adhoc',
                 spyt_driver_filename=driver_script,
                 spyt_driver_args=spyt_args)

    workflow_id = '18866e00-3001-4be8-8c6e-e59b0da2733e' if is_prod and not rebuild else 'ce1681d4-457b-4c74-8d1b-f2cbc0b2e95e'
    vh.run(wait=False,
           keep_going=True,
           start=with_start,
           workflow_guid=workflow_id,
           quota='coud-analytics',
           label='Weekly forecast',
           backend=vh.NirvanaBackend())


if __name__ == '__main__':
    main()
