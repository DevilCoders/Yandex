import vh
import logging.config
from clan_tools.logging.logger import default_log_config
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

    package = get_package(package_path='ml/upsell', local_script=local_src, files_to_copy=['src/', 'scripts/'])

    deps_path = '//home/cloud_analytics/ml/upsell/upsell_and_contact_more_than_70_days/deps'
    res_path = '//home/cloud_analytics/export/crm/update_call_center_leads/update_leads' + ('' if is_prod else '_test')
    history_path = '//home/cloud_analytics/ml/upsell/upsell_and_contact_more_than_70_days/upsell_and_contact_history' + ('' if is_prod else '_test')
    script_generate_leads = 'contact_more_than_70_days.py'

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

    db_on_vm = run_small_job_op(_name='DB on VM info', max_ram=128, input=package, yql_token='robot-clanalytics-yql',
                                script='scripts/run_yql_joblayer.py', script_args=['--path src/upsell/yql/db_on_vm_information.sql'])

    cmp_accs = run_small_job_op(_name='Company accs & leads', max_ram=128, input=package, yql_token='robot-clan-pii-yt-yql_token',
                                script='scripts/run_yql_joblayer.py', script_args=['--path src/upsell/yql/crm_business_accs_and_leads.sql'])

    with vh.wait_for(db_on_vm, cmp_accs, write_src_zip, write_driver):
        spark_op(_name='Spark: run driver',
                 spyt_deps_dir=deps_path,
                 spyt_secret='robot-clan-pii-yt-yt_token',
                 spyt_yt_token='robot-clan-pii-yt-yt_token',
                 retries_on_job_failure=0,
                 spyt_driver_filename=script_generate_leads,
                 spyt_driver_args=[f'--result_path {res_path}', f'--history_path {history_path}'])

    workflow_id = 'c2609c4f-bf14-4b41-bb22-d741d1e287b8' if is_prod else '15bf5194-d74c-464c-b137-4f1800058bfc'
    vh.run(wait=False,
           keep_going=True,
           start=with_start,
           workflow_guid=workflow_id,
           project='upsell',
           quota='coud-analytics',
           label=f'Contact >70days leads ({"prod" if is_prod else "test"})',
           backend=vh.NirvanaBackend())


if __name__ == '__main__':
    main()
