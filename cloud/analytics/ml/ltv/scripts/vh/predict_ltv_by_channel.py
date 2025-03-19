import vh
import click
import logging.config
from clan_tools.logging.logger import default_log_config
from clan_tools.utils.timing import timing
from clan_tools.vh.workflow import get_package, prepare_dependencies
from clan_tools.vh.operations import spark_op, run_small_job_op
from clan_tools.utils.dict import DictObj
from ltv.utils import config 

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

    package = get_package(package_path='ml/ltv', local_script=local_src, files_to_copy=['src/', 'scripts/'])
    paths = DictObj(
        deps_dir='//home/cloud_analytics/ml/ltv/deps',
    )

    dependencies = prepare_dependencies(package, paths.deps_dir, ['collect_data.py'], drivers_local_dir='scripts')

    with vh.wait_for(*dependencies):
        dataset = spark_op(
            _name="Collect Data",
            spyt_deps_dir=paths.deps_dir,
            spyt_driver_filename='collect_data.py'
            )

    with vh.wait_for(dataset):
        run_small_job_op(
            _name='Predict LTV',
            input=package,
            max_ram=128,
            script='scripts/compute_ltv.py',
            yt_token='robot-clanalytics-yt',
            )

    vh.run(
        wait=False,
        keep_going=True,
        start=with_start,
        workflow_guid='8f7f296a-9ce4-4f41-a600-bc07e54cec11' if is_prod else 'a96db027-5b50-4b5a-9943-699d12436968',
        label=f'Collect Frequent Data {"" if is_prod else "test"}',
        quota='coud-analytics',
        backend=vh.NirvanaBackend()
        )

if __name__ == '__main__':
    main()
