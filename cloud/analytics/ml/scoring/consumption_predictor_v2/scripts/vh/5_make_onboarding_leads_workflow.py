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
    package = get_package(package_path='ml/scoring/consumption_predictor_v2', local_script=local_src, files_to_copy=['src/', 'scripts/'])

    # pathes in yt
    deps_path = '//home/cloud_analytics/ml/scoring/consumption_predictor_v2/deps/wf5'
    add_onb_leads_script = '5_make_onboarding_leads.py'

    postfix = "" if is_prod else "_test"
    results_path = "//home/cloud_analytics/ml/scoring/consumption_predictor_v2/data/prod_results"
    features_path = "//home/cloud_analytics/ml/scoring/consumption_predictor_v2/data/actual_features"
    leads_table = "//home/cloud_analytics/export/crm/update_call_center_leads/update_leads" + postfix
    leads_table_history = "//home/cloud_analytics/ml/scoring/consumption_predictor_v2/crm/onboarding/onb_history" + postfix

    # thread #1: deploy.tar -> unpack bin of src folder -> compress to zip -> write it on yt
    src = extract_op(_name='Extract deps', archive=package, out_type='binary', path='src').binary_file
    src_zip = zip_op(_name='deps to ".zip"', input=src).output
    write_src_zip = yt_write_file(_name='deps.zip -> YT', file=src_zip, path=f'{deps_path}/dependencies.zip')

    # thread #2: deploy.tar -> unpack features exec of driver -> write it on yt
    spyt_driver = extract_op(_name='Extract driver script', archive=package, out_type='exec', path=f'scripts/{add_onb_leads_script}').exec_file
    write_spyt_driver = yt_write_file(_name='Driver script -> YT', file=spyt_driver, path=f'{deps_path}/{add_onb_leads_script}')

    # operation: start spyt block on worker
    with vh.wait_for(write_src_zip, write_spyt_driver):
        spark_op(_name='Run spyt-driver',
                 spyt_deps_dir=deps_path,
                 spyt_driver_filename=add_onb_leads_script,
                 retries_on_job_failure=0,
                 ttl=480,
                 max_ram=1024,
                 spyt_secret='robot-clan-pii-yt-yt_token',
                 spyt_yt_token='robot-clan-pii-yt-yt_token',
                 spyt_driver_args=[
                     f'--results_path {results_path}',
                     f'--features_path {features_path}',
                     f'--leads_table {leads_table}',
                     f'--leads_table_history {leads_table_history}'
                 ])

    workflow_id = '37fc4020-0e66-4c70-87aa-9693421f63b9' if is_prod else 'a4151731-335d-4779-8601-7d088b26e792'
    vh.run(wait=False,
           keep_going=True,
           start=with_start,
           workflow_guid=workflow_id,
           project='consumption_predictor_v2',
           quota='coud-analytics',
           label=f'Make Onboarding Leads ({"prod" if is_prod else "test"})',
           backend=vh.NirvanaBackend())


if __name__ == '__main__':
    main()
