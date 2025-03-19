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

    # common operation of folder loading with correction: we write config to src
    package = get_package(package_path='ml/scoring/consumption_predictor_v2', local_script=local_src, files_to_copy=['src/', 'scripts/'])

    # script for every day loading marketing features
    metrike_logs_script = 'scripts/1_collect_marketing_features.py'
    run_small_job_op(_name='Cooked logs', input=package, script=metrike_logs_script, yql_token='robot-clanalytics-yql', script_args=['-n 2'])

    workflow_id = 'd54476c7-c530-42c0-bce1-4c7e0cf595e2' if is_prod else 'a4151731-335d-4779-8601-7d088b26e792'
    vh.run(wait=False,
           keep_going=True,
           start=with_start,
           workflow_guid=workflow_id,
           project='consumption_predictor_v2',
           quota='coud-analytics',
           label=f'Collect marketing features ({"prod" if is_prod else "test"})',
           backend=vh.NirvanaBackend())


if __name__ == '__main__':
    main()
