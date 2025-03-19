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
@click.option('--rebuild', is_flag=True, default=False)
def main(local_src,  is_prod, with_start, rebuild):
    logger.debug('Starting graph')
    logger.info('Updating production workflow' if is_prod else 'Testing workflow')
    logger.info('Workflow will start' if with_start else 'Workflow will not start')

    # common operation of folder loading with correction: we write config to src
    package = get_package(package_path='ml/scoring/consumption_predictor_v2', local_script=local_src, files_to_copy=['src/', 'scripts/'])

    # pathes in yt
    deps_path = '//home/cloud_analytics/ml/scoring/consumption_predictor_v2/deps'
    features_driver_script = 'scripts/1_collect_main_features.py'
    target_driver_script = 'scripts/1_collect_target.py'

    # spyt params
    features_path = "//home/cloud_analytics/ml/scoring/consumption_predictor_v2/data/actual_features"
    target_path = "//home/cloud_analytics/ml/scoring/consumption_predictor_v2/data/actual_target"

    # thread #1: deploy.tar -> unpack bin of src folder -> compress to zip -> write it on yt
    src = extract_op(_name='Extract deps', archive=package, out_type='binary', path='src').binary_file
    src_zip = zip_op(_name='deps to ".zip"', input=src).output
    write_src_zip = yt_write_file(_name='deps.zip -> YT', file=src_zip, path=f'{deps_path}/dependencies.zip')

    # thread #2: deploy.tar -> unpack features exec of driver -> write it on yt
    features_driver = extract_op(_name='Extract features', archive=package, out_type='exec', path=features_driver_script).exec_file
    write_features_driver = yt_write_file(_name='Features script -> YT', file=features_driver, path=f'{deps_path}/{features_driver_script}')

    # thread #3: deploy.tar -> unpack target exec of driver -> write it on yt
    target_driver = extract_op(_name='Extract target', archive=package, out_type='exec', path=target_driver_script).exec_file
    write_target_driver = yt_write_file(_name='Target script -> YT', file=target_driver, path=f'{deps_path}/{target_driver_script}')

    # operation: start spyt block on worker
    with vh.wait_for(write_src_zip, write_features_driver):
        collect_features = spark_op(_name='Spark: Features',
                                    spyt_deps_dir=deps_path,
                                    spyt_driver_filename=features_driver_script,
                                    retries_on_job_failure=0,
                                    spyt_driver_args=[
                                        f'--features_path {features_path}',
                                        '--rebuild' if rebuild else ''
                                    ])

    # operation: start spyt block on worker
    with vh.wait_for(collect_features, write_target_driver):
        spark_op(_name='Spark: Target',
                 spyt_deps_dir=deps_path,
                 spyt_driver_filename=target_driver_script,
                 retries_on_job_failure=0,
                 spyt_driver_args=[
                     f'--features_path {features_path}',
                     f'--target_path {target_path}',
                     '--rebuild' if rebuild else ''
                 ])

    workflow_id = '30641f9e-0dc6-4b19-8412-bae73674fb74' if is_prod else 'a4151731-335d-4779-8601-7d088b26e792'
    vh.run(wait=False,
           keep_going=True,
           start=with_start,
           workflow_guid=workflow_id,
           project='consumption_predictor_v2',
           quota='coud-analytics',
           label=f'Collect features and target ({"prod" if is_prod else "test"})',
           backend=vh.NirvanaBackend())


if __name__ == '__main__':
    main()
