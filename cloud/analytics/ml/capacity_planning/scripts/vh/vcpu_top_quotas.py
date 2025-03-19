import vh
import click
import logging.config
from clan_tools.logging.logger import default_log_config
from clan_tools.vh.workflow import get_package
from clan_tools.vh.operations import run_small_job_op


logging.config.dictConfig(default_log_config)
logger = logging.getLogger(__name__)


@click.command()
@click.option('--local_src', is_flag=True, default=False)
@click.option('--is_prod', is_flag=True, default=False)
@click.option('--with_start', is_flag=True, default=False)
@click.option('--rebuild', is_flag=True, default=False)
def main(local_src: bool, is_prod: bool,  with_start: bool, rebuild: bool) -> None:
    logger.debug('Starting graph')
    logger.info('Workflow will start' if with_start else 'Workflow will not start')
    package = get_package(package_path='ml/capacity_planning', local_script=local_src, files_to_copy=['src/', 'scripts/'])
    args = []
    if is_prod:
        args.append('--is_prod')
    if rebuild:
        args.append('--rebuild')
    run_small_job_op(_name='Top compute quotas', max_ram=128, input=package, script='scripts/vcpu/top_quotas.py',
                     yql_token='robot-clan-pii-yt-yql_token', yt_token='robot-clan-pii-yt-yt_token', script_args=args)

    workflow_id = '0310acd8-b4b2-4401-8704-3fd41169bb10' if is_prod else '9c88f1fe-3d3f-4839-908a-769ae39145d8'
    vh.run(wait=False,
           keep_going=True,
           start=with_start,
           workflow_guid=workflow_id,
           project='resource_management',
           quota='coud-analytics',
           label='Top quotas',
           backend=vh.NirvanaBackend())


if __name__ == '__main__':
    main()
