import vh
import click
import logging.config
from textwrap import dedent
from clan_tools.logging.logger import default_log_config
from clan_tools.vh.workflow import get_package
from clan_tools.secrets.Vault import Vault
from clan_tools.data_adapters.YTAdapter import YTAdapter
from clan_tools.vh.operations import get_mr_dir, solomon_to_yt, run_small_job_op
from clan_tools.vh.operations import extract_op, zip_op, spark_op, yt_write_file


logging.config.dictConfig(default_log_config)
logger = logging.getLogger(__name__)
Vault().get_secrets()


@click.command()
@click.option('--local_src', is_flag=True, default=False)
@click.option('--is_prod', is_flag=True, default=False)
@click.option('--with_start', is_flag=True, default=False)
@click.option('--rebuild', is_flag=True, default=False)
@click.option('--repair', is_flag=True, default=False)
def main(local_src: bool,  is_prod: bool, with_start: bool, rebuild: bool, repair: bool) -> None:
    logger.debug('Starting graph')
    logger.info('Updating production workflow' if is_prod else 'Testing workflow')
    logger.info('Workflow will start' if with_start else 'Workflow will not start')
    logger.info('Source will be rebuilt' if rebuild else 'Source won\'t be rebuilt')

    yt_adapter = YTAdapter()
    raw_imported_data_path = '//home/cloud_analytics/ml/capacity_planning/vcpu/import'
    package = get_package(package_path='ml/capacity_planning', local_script=local_src, files_to_copy=['src/', 'scripts/'])
    envs = ['prod', 'preprod']

    # remove current tables if rebuild
    if rebuild:
        for env in ['prod', 'preprod']:
            source_path = f'{raw_imported_data_path}/{env}/1d'
            source_tables = [f'{source_path}/{table}' for table in yt_adapter.yt.list(source_path)]
            for table in source_tables:
                yt_adapter.yt.remove(table)

    # data transfer from solomon to YT
    folders = {}
    preprocess_ops = []
    for env in envs:
        solomon_selectors = dedent(f"""
            {{project='yandexcloud',
            cluster='cloud_{env}_scheduler',
            service='resources',
            metric='*cores*',
            host_group='all|-',
            platform='*',
            node_name='all_nodes',
            zone_id='ru-central1-*'}}
        """)
        workdir = f'{raw_imported_data_path}/{env}'
        mr = get_mr_dir(_name=f'Folder ({env})', _options={'path': workdir})
        date_from = '2020-02-01' if env == 'prod' else '2021-04-01'
        transfer_op = solomon_to_yt(
            _name=f'vCPU {env} load',
            dst=mr.mr_directory,
            max_ram=256,
            solomon_project='yandexcloud',
            solomon_selectors=dedent(solomon_selectors),
            solomon_default_from_dttm=f'{date_from}T00:00:00+0300',
            yt_tables_split_interval='DAILY'
        )
        folders[env] = workdir

        with vh.wait_for(transfer_op):
            source_path = f'{raw_imported_data_path}/{env}'
            script_args = [f"--workdir {source_path}"]
            if is_prod:
                script_args.append('--is_prod')
            if rebuild:
                script_args.append('--rebuild')
            preprocess_op = run_small_job_op(_name=f'Prepare data ({env})',
                                             max_ram=128, input=package,
                                             script='scripts/vcpu/source_data.py',
                                             yql_token='robot-clan-pii-yt-yql_token',
                                             yt_token='robot-clan-pii-yt-yt_token',
                                             script_args=script_args)
            preprocess_ops.append(preprocess_op)

    with vh.wait_for(*preprocess_ops):
        # thread #1: deploy.tar -> unpack bin of src folder -> compress to zip -> write it on yt
        deps_path = '//home/cloud_analytics/ml/capacity_planning/vcpu/import/deps'
        prepare_script = 'historical_data.py'

        src = extract_op(_name='Extract "src/"', archive=package, out_type='binary', path='src').binary_file
        driver_prepare = extract_op(_name='Extract driver', archive=package, out_type='exec', path=f'scripts/vcpu/{prepare_script}').exec_file

    src_zip = zip_op(_name='Compress to ".zip"', input=src).output
    write_src_zip = yt_write_file(_name='Write on YT', file=src_zip, path=f'{deps_path}/dependencies.zip')
    write_driver_collect = yt_write_file(_name='Write on YT', file=driver_prepare, path=f'{deps_path}/{prepare_script}')

    with vh.wait_for(write_src_zip, write_driver_collect):
        for env in ['prod', 'preprod']:
            source_path = f'{raw_imported_data_path}/{env}'
            spyt_args = [f"--env {env}"]
            if is_prod:
                spyt_args.append('--is_prod')
            if repair:
                spyt_args.append('--repair')
            spark_op(_name=f'Dataset ({env})',
                     spyt_deps_dir=deps_path,
                     spyt_secret='robot-clan-pii-yt-yt_token',
                     spyt_yt_token='robot-clan-pii-yt-yt_token',
                     retries_on_job_failure=0,
                     spyt_driver_filename=prepare_script,
                     spyt_driver_args=spyt_args)

    workflow_id = '2efac264-e6e0-4afc-8f7c-591b19a5c6af' if is_prod else '9c88f1fe-3d3f-4839-908a-769ae39145d8'
    vh.run(wait=False,
           keep_going=True,
           start=with_start,
           workflow_guid=workflow_id,
           project='resource_management',
           quota='coud-analytics',
           label=f'Import and prepare data ({"prod" if is_prod else "test"})',
           backend=vh.NirvanaBackend())


if __name__ == '__main__':
    main()
