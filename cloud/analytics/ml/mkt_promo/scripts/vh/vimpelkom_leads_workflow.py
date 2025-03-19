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

    # common operation of folder loading with correction: we write config to src
    package = get_package(package_path='ml/mkt_promo', local_script=local_src, files_to_copy=['src/', 'scripts/'])

    # pathes in yt
    deps_path_yt = '//home/cloud_analytics/ml/mkt_promo/deps'
    vimpelkom_leads_script = 'vimpelkom_leads.py'

    # thread #1: deploy.tar -> unpack bin of src folder -> compress to zip -> write it on yt
    src = extract_op(_name='Extract deps', archive=package, out_type='binary', path='src').binary_file
    src_zip = zip_op(_name='deps to ".zip"', input=src).output
    write_src_zip = yt_write_file(_name='deps.zip -> YT', file=src_zip, path=f'{deps_path_yt}/dependencies.zip')

    # thread #2: deploy.tar -> unpack features exec of driver -> write it on yt
    spyt_driver = extract_op(_name='Extract driver script', archive=package, out_type='exec', path=f'scripts/{vimpelkom_leads_script}').exec_file
    write_spyt_driver = yt_write_file(_name='Driver script -> YT', file=spyt_driver, path=f'{deps_path_yt}/{vimpelkom_leads_script}')

    # operation: start spyt block on worker
    with vh.wait_for(write_src_zip, write_spyt_driver):
        spyt_args = []
        if is_prod:
            spyt_args.append('--is_prod')
        spark_op(_name='Run spyt-driver',
                 spyt_deps_dir=deps_path_yt,
                 spyt_driver_filename=vimpelkom_leads_script,
                 retries_on_job_failure=0,
                 spyt_yt_token='robot-clan-pii-yt-yt_token',
                 spyt_secret='robot-clan-pii-yt-yt_token',
                 spyt_driver_args=spyt_args)

    workflow_id = '0934e92b-03c9-4afd-9316-d43877570917' if is_prod else '47751c69-0da0-4b6d-855f-54a9117bd4fd'
    vh.run(wait=False,
           keep_going=True,
           start=with_start,
           workflow_guid=workflow_id,
           project='vimpelkom_leads',
           quota='coud-analytics',
           label=f'Vimpelkom leads ({"prod" if is_prod else "test"})',
           backend=vh.NirvanaBackend())


if __name__ == '__main__':
    main()
