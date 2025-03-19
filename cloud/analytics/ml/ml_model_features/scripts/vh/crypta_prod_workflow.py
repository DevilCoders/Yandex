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
@click.option('--rebuild', is_flag=True, default=False)
def main(local_src: bool = False,  is_prod: bool = False, with_start: bool = False, rebuild: bool = False) -> None:
    logger.debug('Starting graph')
    logger.info('Updating production workflow' if is_prod else 'Testing workflow')
    logger.info('Workflow will start' if with_start else 'Workflow will not start')

    package = get_package(package_path='ml/ml_model_features', local_script=local_src, files_to_copy=['src/', 'scripts/'])

    run_small_job_op(
        _name='Crypta prod table',
        input=package,
        max_ram=128,
        script='scripts/crypta/crypta_prod.py',
        yt_token='robot-clanalytics-yt',
        yql_token='robot-clanalytics-yql',
        script_args=['--rebuild' if rebuild else '']
    )

    vh.run(
        wait=False,
        keep_going=True,
        start=with_start,
        workflow_guid=('c5214bc2-edf6-4f4f-8e8c-ba2dd8043759' if is_prod else None),
        quota='coud-analytics',
        backend=vh.NirvanaBackend()
    )


if __name__ == '__main__':
    main()
