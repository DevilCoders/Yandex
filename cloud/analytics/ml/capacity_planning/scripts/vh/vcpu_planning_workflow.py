import vh
import click
import logging.config
from clan_tools.logging.logger import default_log_config
from clan_tools.vh.workflow import get_package
from clan_tools.vh.operations import extract_op, zip_op, spark_op, yt_write_file

logging.config.dictConfig(default_log_config)
logger = logging.getLogger(__name__)


@click.command()
@click.option('--local_src', is_flag=True, default=False)
@click.option('--is_prod', is_flag=True, default=False)
@click.option('--with_start', is_flag=True, default=False)
def main(local_src: bool,  is_prod: bool, with_start: bool) -> None:
    logger.debug('Starting graph')
    logger.info('Updating production workflow' if is_prod else 'Testing workflow')
    logger.info('Workflow will start' if with_start else 'Workflow will not start')

    package = get_package(package_path='ml/capacity_planning', local_script=local_src, files_to_copy=['src/', 'scripts/'])

    deps_path = '//home/cloud_analytics/ml/capacity_planning/vcpu/deps'
    script_predict_data = 'forecast_data.py'

    # thread #1: deploy.tar -> unpack bin of src folder -> compress to zip -> write it on yt
    src = extract_op(_name='Extract "src/"', archive=package, out_type='binary', path='src').binary_file
    src_zip = zip_op(_name='Compress to ".zip"', input=src).output
    write_src_zip = yt_write_file(_name='Write on YT', file=src_zip, path=f'{deps_path}/dependencies.zip')

    driver_predict = extract_op(_name='Extract driver', archive=package, out_type='exec',
                                 path=f'scripts/vcpu/{script_predict_data}').exec_file
    write_driver_predict = yt_write_file(_name='Write on YT', file=driver_predict, path=f'{deps_path}/{script_predict_data}')

    with vh.wait_for(write_src_zip, write_driver_predict):
        for env in ['prod', 'preprod']:
            spyt_predict_args = [f'--env {env}']
            if is_prod:
                spyt_predict_args.append('--is_prod')

            spark_op(_name=f'Forecast data ({env})',
                     spyt_deps_dir=deps_path,
                     spyt_secret='robot-clan-pii-yt-yt_token',
                     spyt_yt_token='robot-clan-pii-yt-yt_token',
                     retries_on_job_failure=0,
                     spyt_driver_filename=script_predict_data,
                     spyt_driver_args=spyt_predict_args)

    workflow_id = '1beb2270-3213-410e-a9e7-80750a7b057a' if is_prod else '9c88f1fe-3d3f-4839-908a-769ae39145d8'
    vh.run(wait=False,
           keep_going=True,
           start=with_start,
           workflow_guid=workflow_id,
           project='resource_management',
           quota='coud-analytics',
           label=f'Capacity planning - vCPU ({"prod" if is_prod else "test"})',
           backend=vh.NirvanaBackend())


if __name__ == '__main__':
    main()
