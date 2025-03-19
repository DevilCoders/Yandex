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

    # default parameters from code
    nbs_prod_used_path = "//home/cloud_analytics/dwh/ods/nbs/nbs_disk_used_space"
    nbs_prod_purch_path = "//home/cloud_analytics/dwh/ods/nbs/nbs_disk_purchased_space"
    nbs_preprod_used_path = "//home/cloud_analytics/dwh_preprod/ods/nbs/nbs_disk_used_space"
    nbs_preprod_purch_path = "//home/cloud_analytics/dwh_preprod/ods/nbs/nbs_disk_purchased_space"
    result_table_path = "//home/cloud_analytics/resources_overbooking/forecast-nbs"

    forecast_nbs_path = "//home/cloud_analytics/resources_overbooking/forecast-nbs"
    forecast_kkr_nrd_path = "//home/cloud_analytics/resources_overbooking/forecast-kkr-nrd"
    container_limits_path = "//home/cloud_analytics/resources_overbooking/container_limits"
    detailed_nbs_path = "//home/cloud_analytics/resources_overbooking/nbs"
    mon_result_table_path = "//home/cloud_analytics/resources_overbooking/disk_capacity_monitoring"

    # common operation of folder loading with correction: we write config to src
    package = get_package(package_path='ml/resources_overbooking',
                          local_script=local_src,
                          files_to_copy=['src/', 'scripts/'])

    # pathes in yt
    deps_path = '//home/cloud_analytics/resources_overbooking/deps'
    forecast_resources_script = 'scripts/disk_capacity_nbs.py'

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
        run_driver = spark_op(_name='Spark: run driver',
                              spyt_deps_dir=deps_path,
                              spyt_driver_filename=forecast_resources_script,
                              spyt_driver_args=[
                                  f'--nbs_prod_used_path {nbs_prod_used_path}',
                                  f'--nbs_prod_purch_path {nbs_prod_purch_path}',
                                  f'--nbs_preprod_used_path {nbs_preprod_used_path}',
                                  f'--nbs_preprod_purch_path {nbs_preprod_purch_path}',
                                  f'--result_table_path {result_table_path}',
                                  '--predict_from None',
                                  '--predict_to None'
                              ])

    with vh.wait_for(run_driver):
        run_small_job_op(_name='Table for monitoring',
                         max_ram=128, input=package,
                         script='scripts/disk_monitoring.py',
                         yql_token='robot-clanalytics-yql',
                         script_args=[
                             f'--forecast_nbs_path {forecast_nbs_path}',
                             f'--forecast_kkr_nrd_path {forecast_kkr_nrd_path}',
                             f'--container_limits_path {container_limits_path}',
                             f'--detailed_nbs_path {detailed_nbs_path}',
                             f'--result_table_path {mon_result_table_path}'
                         ])
        for env_flag in [True, False]:
            run_small_job_op(_name=f"NBS & Quota ({'prod' if env_flag else 'preprod'})", max_ram=128,
                             input=package, script='scripts/disk_monitoring_quota_and_nbs.py',
                             yql_token='robot-clanalytics-yql', yt_token='robot-clanalytics-yt',
                             script_args=[
                                 '--env_flag' if env_flag else '',
                                 f'--detailed_nbs_path {detailed_nbs_path}',
                                 '--result_table //home/cloud_analytics/resources_overbooking/monitoring_quota_and_nbs'
                             ])

    workflow_id = 'abb563dc-c0ce-4d70-a1d6-5a793ecda81e' if is_prod else 'cc471f52-f2cd-4f9b-8330-61097500dbda'
    vh.run(wait=False,
           keep_going=True,
           start=with_start,
           workflow_guid=workflow_id,
           project='resource_management',
           quota='coud-analytics',
           label=f'Nbs forecast ({"prod" if is_prod else "test"})',
           backend=vh.NirvanaBackend())


if __name__ == '__main__':
    main()
