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
    package = get_package(package_path='ml/scoring/trial_to_paid', local_script=local_src, files_to_copy=['src/', 'scripts/'])

    # pathes in yt
    deps_path = '//home/cloud_analytics/ml/scoring/consumption_predictor_v2/deps'
    leads_export = 'scripts/leads_export_script.py'

    # thread #1: deploy.tar -> unpack bin of src folder -> compress to zip -> write it on yt
    src = extract_op(_name='Extract deps', archive=package, out_type='binary', path='src').binary_file
    src_zip = zip_op(_name='deps to ".zip"', input=src).output
    write_src_zip = yt_write_file(_name='deps.zip -> YT', file=src_zip, path=f'{deps_path}/dependencies.zip')

    # thread #2: deploy.tar -> unpack features exec of driver -> write it on yt
    spyt_driver = extract_op(_name='Extract driver script', archive=package, out_type='exec', path=leads_export).exec_file
    write_spyt_driver = yt_write_file(_name='Driver script -> YT', file=spyt_driver, path=f'{deps_path}/{leads_export}')

    # operation: start spyt block on worker
    with vh.wait_for(write_src_zip, write_spyt_driver):
        spark_op(_name='Export leads',
                 spyt_deps_dir=deps_path,
                 spyt_driver_filename=leads_export)

    workflow_id = 'd2b0eef1-549b-446a-9ebe-75e87740d418' if is_prod else '97d30485-13b0-4449-ab0a-b8462874a364'
    vh.run(wait=False,
           keep_going=True,
           start=with_start,
           workflow_guid=workflow_id,
           project='trial_to_paid',
           quota='coud-analytics',
           label=f'Export Leads ({"prod" if is_prod else "test"})',
           backend=vh.NirvanaBackend())


if __name__ == '__main__':
    main()
