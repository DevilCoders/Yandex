import vh
import logging.config
from clan_tools.logging.logger import default_log_config
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

    package = get_package(package_path='ml/leads_flow', local_script=local_src, files_to_copy=['src/', 'scripts/'])

    deps_path = '//home/cloud_analytics/ml/leads_flow/deps'
    script_leads_flow = 'update_leads_flow.py'

    # thread #1: deploy.tar -> unpack bin of src folder -> compress to zip -> write it on yt
    src = extract_op(_name='Extract "src/"', archive=package, out_type='binary', path='src').binary_file
    src_zip = zip_op(_name='Compress to ".zip"', input=src).output
    write_src_zip = yt_write_file(_name='Write on YT', file=src_zip, path=f'{deps_path}/dependencies.zip')

    # thread #2: deploy.tar -> unpack exec of driver -> write it on yt
    driver = extract_op(_name='Extract driver',
                        archive=package,
                        out_type='exec',
                        path=f'scripts/{script_leads_flow}').exec_file
    write_driver = yt_write_file(_name='Write on YT', file=driver, path=f'{deps_path}/{script_leads_flow}')

    with vh.wait_for(write_src_zip, write_driver):
        spark_op(_name=f"Leads flow ({'prod' if is_prod else 'test'})",
                 spyt_deps_dir=deps_path,
                 spyt_secret='robot-clan-pii-yt-yt_token',
                 spyt_yt_token='robot-clan-pii-yt-yt_token',
                 retries_on_job_failure=0,
                 cluster='adhoc',
                 spyt_driver_filename=script_leads_flow,
                 spyt_driver_args=['--is_prod' if is_prod else ''])

    workflow_id = '732835f7-6cae-422d-8daa-028fb0c306d5' if is_prod else 'ab9e8ee4-243a-4777-b1c3-21b4470bc0a9'
    vh.run(wait=False,
           keep_going=True,
           start=with_start,
           workflow_guid=workflow_id,
           project='leads_flow',
           quota='coud-analytics',
           label=f'Leads Flow ({"prod" if is_prod else "test"})',
           backend=vh.NirvanaBackend())


if __name__ == '__main__':
    main()
