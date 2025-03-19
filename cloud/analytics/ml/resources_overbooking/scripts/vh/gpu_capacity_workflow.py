import vh
from clan_tools.logging.logger import default_log_config
import logging.config
from textwrap import dedent
from clan_tools.utils.timing import timing
from clan_tools.vh.workflow import get_package
from clan_tools.vh.operations import get_mr_dir, solomon_to_yt, run_small_job_op
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

    # data transfer from solomon to YT
    transfer_ops = []
    folders = {}
    for env in ['prod', 'preprod']:
        raw_imported_data_path = '//home/cloud_analytics/resources_overbooking/gpu/import/solomon'
        solomon_selectors = f"""
            {{project='yandexcloud',
            cluster='cloud_{env}_scheduler',
            service='resources',
            metric='gpu_free|gpu_total',
            host_group='-|all',
            model='*',
            node_name='all_nodes',
            platform='gpu-standard-v1|gpu-standard-v2|gpu-standard-v3|standard-v3-t4',
            zone_id='ru-central1-*'}}
        """
        workdir = f'{raw_imported_data_path}/{env}'
        mr = get_mr_dir(_name=f'Folder ({env})', _options={'path': workdir})
        transfer_op = solomon_to_yt(
            _name=f'GPU {env} load',
            dst=mr.mr_directory,
            max_ram=256,
            solomon_project='yandexcloud',
            solomon_selectors=dedent(solomon_selectors),
            solomon_default_from_dttm='2021-12-02T00:00:00+0300',
            yt_tables_split_interval='DAILY'
        )
        transfer_ops.append(transfer_op)
        folders[env] = workdir

    package = get_package(package_path='ml/resources_overbooking', local_script=local_src, files_to_copy=['src/', 'scripts/'])

    run_small_job_op(_name='Daily (Billing)', max_ram=128, input=package, yql_token='robot-clanalytics-yql',
                     script='scripts/run_yql_joblayer.py', script_args=['--path src/resources_overbooking/gpu/yql/gpu_resources_from_billing.sql'])

    result_table = '//home/cloud_analytics/resources_overbooking/gpu/daily_table'
    with vh.wait_for(*transfer_ops):
        run_small_job_op(_name='Daily (Solomon)',
                         max_ram=128, input=package,
                         script='scripts/gpu_capacity.py',
                         yql_token='robot-clanalytics-yql',
                         yt_token='robot-clanalytics-yt',
                         script_args=[
                             f"-r {result_table}",
                             f"-p {folders['prod']}/1d",
                             f"-e {folders['preprod']}/1d"
                         ])

    workflow_id = '5866452b-0ef8-428a-9757-5ab91437785f' if is_prod else '370c89bb-fcf7-4db7-b475-59ab1febdd17'
    vh.run(wait=False,
           keep_going=True,
           start=with_start,
           workflow_guid=workflow_id,
           project='resource_management',
           quota='coud-analytics',
           label=f'GPU daily tables ({"prod" if is_prod else "test"})',
           backend=vh.NirvanaBackend())


if __name__ == '__main__':
    main()
