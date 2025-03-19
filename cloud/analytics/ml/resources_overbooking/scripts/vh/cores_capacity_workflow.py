import vh
from clan_tools.vh.workflow import get_package
from clan_tools.vh.operations import extract_op, zip_op
from clan_tools.vh.operations import spark_op, yt_write_file, run_small_job_op
from clan_tools.utils.timing import timing
from clan_tools.logging.logger import default_log_config
import click
import logging.config

logging.config.dictConfig(default_log_config)
logger = logging.getLogger(__name__)


@timing
@click.command()
@click.option('--prod', is_flag=True)
@click.option('--no_start', is_flag=True)
@click.option('--local_src', is_flag=True)
@click.option('--update_prod', is_flag=True)
def main(local_src=True,  prod=False, no_start=False, update_prod=False):
    logger.info('Starting graph')
    logger.info('Updating prod workflow' if update_prod else 'New workflow was created')
    logger.info('Workflow will start' if (not no_start) else 'Workflow will not start')

    package = get_package(package_path='ml/resources_overbooking', local_script=local_src, files_to_copy=['scripts/'])

    # pathes in yt
    deps_path = '//home/cloud_analytics/resources_overbooking/deps'
    cores_monitoring_script = 'scripts/cores_monitoring.py'
    cores_capacity_script = 'scripts/cores_capacity.py'

    # thread #1: make spyt-dependencies
    #   deploy.tar -> unpack bin of src folder -> compress bin to zip -> save zip-dependencies in YT
    src = extract_op(_name='Extract .bin of deps', archive=package, out_type='binary', path='src').binary_file
    src_zip = zip_op(_name='Compress deps in .zip', input=src).output
    write_src_zip = yt_write_file(_name='Save deps on YT', file=src_zip, path=f'{deps_path}/dependencies.zip')

    # thread #2: make spyt-driver with forecast code
    #   deploy.tar -> unpack exec of forecast driver -> write it on YT
    driver = extract_op(_name='Extract forecast driver', archive=package,
                        out_type='exec', path=cores_capacity_script).exec_file
    write_driver = yt_write_file(_name='Save it on YT', file=driver, path=f'{deps_path}/{cores_capacity_script}')

    # thread #3: make historical data
    #   deploy.tar -> unpack data collection driver -> write it on YT
    driver_cores = extract_op(_name='Extract data driver', archive=package,
                              out_type='exec', path=cores_monitoring_script).exec_file
    write_driver_cores = yt_write_file(_name='Save it on YT',
                                       file=driver_cores, path=f'{deps_path}/{cores_monitoring_script}')

    # run spyt-driver with data collection
    with vh.wait_for(write_src_zip, write_driver_cores):
        res_target = spark_op(_name='Spark: run driver', spyt_deps_dir=deps_path,
                              spyt_driver_filename=cores_monitoring_script)

    # run spyt-driver with forecast
    with vh.wait_for(res_target, write_driver):
        run_driver = spark_op(_name='Spark: run driver', spyt_deps_dir=deps_path,
                              spyt_driver_filename=cores_capacity_script)

    # run porto to collect preemptible cores
    with vh.wait_for(run_driver):
        run_small_job_op(_name='Preemptible monitoring', input=package, max_ram=128,
                         script='scripts/cores_preemptible_monitoring.py', yql_token='robot-clanalytics-yql')

    # make graph
    workflow_id = '95fe6279-e439-4b5a-b23d-df5fc64c5463' if update_prod else 'cc471f52-f2cd-4f9b-8330-61097500dbda'
    vh.run(wait=False,
           keep_going=True,
           start=(not no_start) and (not update_prod),
           workflow_guid=workflow_id,
           project='resource_management', quota='coud-analytics',
           label=f'resource monitoring ({"prod" if prod else "test"})',
           backend=vh.NirvanaBackend())


if __name__ == '__main__':
    main()
