import vh
from clan_tools.vh.workflow import get_package
from clan_tools.vh.operations import preprocess_args
from clan_tools.utils.timing import timing
from clan_tools.logging.logger import default_log_config
import click
import logging.config

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

    package = get_package(package_path='python/cpu_and_mem', local_script=local_src, files_to_copy=['scripts/'])
    run_job_op = preprocess_args(vh.op(
        id='f20381e9-b282-4aa5-92c4-e6ea8b79be25'
    ))
    run_job_op(_name='percentiles.py', input=package, max_ram=1024*2,
               script='scripts/percentiles.py', yt_token='robot-clanalytics-yt', solomon_token='robot-clanalytics-solomon')

    # make graph
    workflow_id = '48bbda9d-2b32-4e01-8079-d641d1260373' if is_prod else 'b37f15ef-2428-4827-8088-458839cb986b'
    vh.run(wait=False,
           keep_going=True,
           start=with_start,
           workflow_guid=workflow_id,
           project='Internal_processes-resource_consumption',
           quota='coud-analytics',
           label=f'cpu_and_mem_({"prod" if is_prod else "test"})',
           backend=vh.NirvanaBackend())


if __name__ == '__main__':
    main()
