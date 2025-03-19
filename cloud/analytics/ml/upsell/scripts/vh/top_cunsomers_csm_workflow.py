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

    package = get_package(package_path='ml/upsell', local_script=local_src, files_to_copy=['src/', 'scripts/'])

    prod_postfix = '' if is_prod else '_test'
    deps_path = '//home/cloud_analytics/ml/upsell/csm_top_cunsomers_leads/deps'
    leads_table = "//home/cloud_analytics/export/crm/update_call_center_leads/update_leads" + prod_postfix
    leads_table_history = "//home/cloud_analytics/ml/upsell/csm_top_cunsomers_leads/csm_top_cunsomers_history" + prod_postfix
    exclude_csm_leads = "//home/cloud_analytics/ml/scoring/consumption_predictor_v2/crm/upsell/csm_history" + prod_postfix
    script_generate_leads = 'top_cunsomers_csm.py'

    # thread #1: deploy.tar -> unpack bin of src folder -> compress to zip -> write it on yt
    src = extract_op(_name='Extract "src/"', archive=package, out_type='binary', path='src').binary_file
    src_zip = zip_op(_name='Compress to ".zip"', input=src).output
    write_src_zip = yt_write_file(_name='Write on YT', file=src_zip, path=f'{deps_path}/dependencies.zip')

    # thread #2: deploy.tar -> unpack exec of driver -> write it on yt
    driver = extract_op(_name='Extract driver',
                        archive=package,
                        out_type='exec',
                        path=f'scripts/{script_generate_leads}').exec_file
    write_driver = yt_write_file(_name='Write on YT', file=driver, path=f'{deps_path}/{script_generate_leads}')

    with vh.wait_for(write_src_zip, write_driver):
        spark_op(_name='Top Cunsomers (CSM)',
                 spyt_deps_dir=deps_path,
                 spyt_secret='robot-clan-pii-yt-yt_token',
                 spyt_yt_token='robot-clan-pii-yt-yt_token',
                 retries_on_job_failure=0,
                 spyt_driver_filename=script_generate_leads,
                 spyt_driver_args=[
                     f'--leads_table {leads_table}',
                     f'--leads_table_history {leads_table_history}',
                     f'--exclude_csm_leads {exclude_csm_leads}'
                 ])

    workflow_id = '7e9ba091-ac58-4e2c-b808-3bd598ea4de7' if is_prod else '4f8ed00e-7e4d-4750-8d30-5d3f21400f86'
    vh.run(wait=False,
           keep_going=True,
           start=with_start,
           workflow_guid=workflow_id,
           project='upsell',
           quota='coud-analytics',
           label=f'Top Cunsomers (CSM) ({"prod" if is_prod else "test"})',
           backend=vh.NirvanaBackend())


if __name__ == '__main__':
    main()
