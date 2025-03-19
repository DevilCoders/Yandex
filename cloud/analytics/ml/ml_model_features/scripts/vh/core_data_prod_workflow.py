import vh
from clan_tools.logging.logger import default_log_config
import logging.config
from clan_tools.utils.timing import timing
from clan_tools.vh.workflow import get_package
from clan_tools.vh.operations import run_small_job_op, run_yql_script
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
    params = run_small_job_op(
        _name='Dates',
        input=package,
        max_ram=128,
        script='scripts/core/generate_dates.py',
        yt_token='robot-clanalytics-yt',
        yql_token='robot-clanalytics-yql',
        script_args=['--rebuild' if rebuild else '']
    )

    run_yql_script(path='src/ml_model_features/yql/payments.sql', parameters=params, _name='Payments')
    run_yql_script(path='src/ml_model_features/yql/vm_usage.sql', parameters=params, _name='VM_usage')

    run_small_job_op(
        _name='Consumption',
        input=package,
        max_ram=128,
        script='scripts/core/consumption_prod.py',
        yt_token='robot-clanalytics-yt',
        yql_token='robot-clanalytics-yql',
        script_args=['--rebuild' if rebuild else '']
    )

    vh.run(
        wait=False,
        keep_going=True,
        start=with_start,
        workflow_guid=('b50a1882-2cf3-4d05-8ab3-9fda36bc77f4' if is_prod else None),
        quota='coud-analytics',
        backend=vh.NirvanaBackend()
    )


if __name__ == '__main__':
    main()
