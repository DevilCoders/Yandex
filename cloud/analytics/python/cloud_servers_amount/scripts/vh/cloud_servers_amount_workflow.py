import vh
from clan_tools.logging.logger import default_log_config
import logging.config
from clan_tools.utils.timing import timing
from clan_tools.vh.workflow import get_package
from clan_tools.vh.operations import extract_op, zip_op, spark_op, yt_write_file, run_small_job_op
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
    package = get_package(package_path='python/cloud_servers_amount', local_script=local_src, files_to_copy=['src/', 'scripts/'])

    deps_path = '//home/cloud_analytics/import/cloud_servers_info/deps'
    load_data_script = 'scripts/load_data.py'
    result_table_path = "//home/cloud_analytics/import/cloud_servers_info/resources_info"

    # thread #1: deploy.tar -> unpack bin of src folder -> compress to zip -> write it on yt
    src = extract_op(_name='Extract deps', archive=package, out_type='binary', path='src').binary_file
    src_zip = zip_op(_name='deps to ".zip"', input=src).output
    write_src_zip = yt_write_file(_name='deps.zip -> YT', file=src_zip, path=f'{deps_path}/dependencies.zip')

    # thread #2: deploy.tar -> unpack features exec of driver -> write it on yt
    spyt_driver = extract_op(_name='Extract driver script', archive=package, out_type='exec', path=load_data_script).exec_file
    write_spyt_driver = yt_write_file(_name='Driver script -> YT', file=spyt_driver, path=f'{deps_path}/{load_data_script}')

    # operation: start spyt block on worker
    with vh.wait_for(write_src_zip, write_spyt_driver):
        load_and_save_data = spark_op(_name='Run spyt-driver',
                                      spyt_deps_dir=deps_path,
                                      retries_on_job_failure=0,
                                      spyt_driver_filename=load_data_script,
                                      spyt_driver_args=[f'--result_table_path {result_table_path}'])

    with vh.wait_for(load_and_save_data):
        sort_result = run_small_job_op(_name='Sorting result', max_ram=128, input=package,
                                       yql_token='robot-clanalytics-yql', script='scripts/run_yql_joblayer.py',
                                       script_args=['--path src/cloud_servers_amount/yql/sort_result_table.sql'])

    with vh.wait_for(sort_result):
        run_small_job_op(_name='Backuping result', max_ram=128, input=package,
                         yql_token='robot-clanalytics-yql', script='scripts/run_yql_joblayer.py',
                         script_args=['--path src/cloud_servers_amount/yql/backup_result_table.sql'])

    workflow_id = '22922019-38af-4bf5-80e5-25f51e08d310' if is_prod else '807fc6f4-c633-4ab5-9f68-331a5b89114f'
    vh.run(wait=False,
           keep_going=True,
           start=with_start,
           workflow_guid=workflow_id,
           project='cloud_servers_amount',
           quota='coud-analytics',
           backend=vh.NirvanaBackend())


if __name__ == '__main__':
    main()
