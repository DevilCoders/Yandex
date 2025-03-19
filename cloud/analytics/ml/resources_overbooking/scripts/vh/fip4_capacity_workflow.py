import vh
from clan_tools.logging.logger import default_log_config
import logging.config
from textwrap import dedent
from clan_tools.utils.timing import timing
from clan_tools.vh.workflow import get_package
from clan_tools.vh.operations import get_mr_dir, solomon_to_yt, run_small_job_op
from clan_tools.vh.operations import extract_op, zip_op, spark_op, yt_write_file
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
        raw_imported_data_path = '//home/cloud_analytics/resources_overbooking/fip4/import/solomon'
        solomon_selectors = f"""
            {{project='yandexcloud',
            cluster='cloud_{env}_vpc-api',
            service='vpc-config-plane',
            host='cluster',
            sensor='resources_allocator_usage',
            metric='used|total',
            allocator='fip4',
            resource_id='public*|ukraine*|qrator*|smtp*|yabank*|vox*'}}
        """
        workdir = f'{raw_imported_data_path}/{env}'
        mr = get_mr_dir(_name=f'Folder ({env})', _options={'path': workdir})
        transfer_op = solomon_to_yt(
            _name=f'FIP4 {env} load',
            dst=mr.mr_directory,
            max_ram=256,
            solomon_project='yandexcloud',
            solomon_selectors=dedent(solomon_selectors),
            solomon_default_from_dttm='2020-11-01T00:00:00+0300',
            yt_tables_split_interval='DAILY'
        )
        transfer_ops.append(transfer_op)
        folders[env] = workdir

    package = get_package(package_path='ml/resources_overbooking', local_script=local_src, files_to_copy=['src/', 'scripts/'])
    result_table_solomon = '//home/cloud_analytics/resources_overbooking/fip4/daily_table'
    result_table_consumption = '//home/cloud_analytics/resources_overbooking/fip4/daily_table_consumption'
    with vh.wait_for(*transfer_ops):
        data_collect = run_small_job_op(_name='FIP4 daily table',
                                        max_ram=128, input=package,
                                        script='scripts/fip4_capacity_data_collect.py',
                                        yql_token='robot-clanalytics-yql',
                                        yt_token='robot-clanalytics-yt',
                                        script_args=[
                                            f"--result_table_solomon {result_table_solomon}",
                                            f"--result_table_consumption {result_table_consumption}",
                                            f"--prod_source_path {folders['prod']}/1d",
                                            f"--preprod_source_path {folders['preprod']}/1d"
                                        ])

    forecast_resources_script = 'scripts/fip4_capacity_forecast.py'

    # pathes in yt
    deps_path = '//home/cloud_analytics/resources_overbooking/deps'
    fip4_consumtion_path = '//home/cloud_analytics/resources_overbooking/fip4/daily_table_consumption'
    result_table_path = '//home/cloud_analytics/resources_overbooking/forecast-fip4'

    # thread #1: deploy.tar -> unpack bin of src folder -> compress to zip -> write it on yt
    src = extract_op(_name='Extract "src/"', archive=package, out_type='binary', path='src').binary_file
    src_zip = zip_op(_name='Compress to ".zip"', input=src).output
    write_src_zip = yt_write_file(_name='Write on YT', file=src_zip, path=f'{deps_path}/dependencies.zip')

    # thread #2: deploy.tar -> unpack exec of driver -> write it on yt
    driver = extract_op(_name='Extract driver',
                        archive=package,
                        out_type='exec',
                        path=forecast_resources_script).exec_file
    write_driver = yt_write_file(_name='Write on YT', file=driver, path=f'{deps_path}/{forecast_resources_script}')

    # final operation: start spyt block on worker
    with vh.wait_for(write_src_zip, write_driver, data_collect):
        spark_op(_name='Spark: run calcs',
                 spyt_deps_dir=deps_path,
                 spyt_driver_filename=forecast_resources_script,
                 spyt_driver_args=[
                     f'--fip4_consumtion_path {fip4_consumtion_path}',
                     f'--result_table_path {result_table_path}'
                 ])

    workflow_id = '56866fd0-3423-40ce-a71d-8053cd83e878' if is_prod else '85687608-0122-45ba-a635-b4c1cf7b3c13'
    vh.run(wait=False,
           keep_going=True,
           start=with_start,
           workflow_guid=workflow_id,
           project='resource_management',
           quota='coud-analytics',
           label=f'Fip4 forecast ({"prod" if is_prod else "test"})',
           backend=vh.NirvanaBackend())


if __name__ == '__main__':
    main()
