import vh
from clan_tools.logging.logger import default_log_config
import logging.config
from clan_tools.utils.timing import timing
from clan_tools.vh.workflow import get_package
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

    # default parameters from code
    kkr_prod_path = "//home/cloud_analytics/dwh/ods/nbs/kikimr_disk_used_space"
    kkr_preprod_path = "//home/cloud_analytics/dwh_preprod/ods/nbs/kikimr_disk_used_space"
    nrd_prod_path = "//home/cloud_analytics/dwh/ods/nbs/nbs_nrd_used_space"
    result_table_path = "//home/cloud_analytics/resources_overbooking/forecast-kkr-nrd"

    # common operation of folder loading with correction: we write config to src
    package = get_package(package_path='ml/resources_overbooking',
                          local_script=local_src,
                          files_to_copy=['src/', 'scripts/'])

    # pathes in yt
    deps_path = '//home/cloud_analytics/resources_overbooking/deps'
    forecast_resources_script = 'scripts/disk_capacity_kkr_nrd.py'

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
    with vh.wait_for(write_src_zip, write_driver):
        spark_op(_name='Spark: run calcs',
                 spyt_deps_dir=deps_path,
                 spyt_driver_filename=forecast_resources_script,
                 spyt_driver_args=[
                     f'--kkr_prod_path {kkr_prod_path}',
                     f'--kkr_preprod_path {kkr_preprod_path}',
                     f'--nrd_prod_path {nrd_prod_path}',
                     f'--result_table_path {result_table_path}',
                     '--predict_from None',
                     '--predict_to None'
                 ])

    workflow_id = '9bdb52bc-7561-41d6-8d8a-01878a24fa6a' if is_prod else 'cc471f52-f2cd-4f9b-8330-61097500dbda'
    vh.run(wait=False,
           keep_going=True,
           start=with_start,
           workflow_guid=workflow_id,
           project='resource_management',
           quota='coud-analytics',
           label=f'Capacity planning ({"prod" if is_prod else "test"})',
           backend=vh.NirvanaBackend())


if __name__ == '__main__':
    main()
