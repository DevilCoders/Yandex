import vh
import click
import logging.config
from clan_tools.logging.logger import default_log_config
from clan_tools.utils.timing import timing
from clan_tools.vh.workflow import get_package, prepare_dependencies
from clan_tools.vh.operations import run_small_job_op
from clan_tools.utils.dict import DictObj
from clan_tools.data_adapters.YTAdapter import YTAdapter

logging.config.dictConfig(default_log_config)
logger = logging.getLogger(__name__)


@timing
@click.command()
@click.option('--no_start', is_flag=True)
@click.option('--local_src', is_flag=True)
@click.option('--update_prod', is_flag=True)
def main(local_src=False, no_start=False, update_prod=False):
    logger.debug('Starting graph')
    logger.info('Updating production workflow' if update_prod else 'Testing workflow')
    logger.info('Workflow will start' if not no_start else 'Workflow will not start')

    # common operation of folder loading with correction: we write config to src
    package = get_package(package_path='ml/ml_model_features', local_script=local_src, files_to_copy=['src/', 'scripts/'])

    deps_path='//home/cloud_analytics/ml/ml_model_features/deps_search_requests'
    YTAdapter().create_paths([deps_path])

    drivers = DictObj(
        cold_and_new_script='search/collect_cold_and_new_search_requests.py',
        recent_script='search/collect_recent_search_requests.py',
        collect_puids_script='collect_puids.py'
    )

    dependencies = prepare_dependencies(
        package, deps_path, drivers.values(), drivers_local_dir='scripts')

    with vh.wait_for(*dependencies):
        puids_collection = run_small_job_op(
            _name='Collect puids',
            input=package,
            max_ram=128,
            script='scripts/' + drivers.collect_puids_script, yql_token='robot-clanalytics-yql'
            )

    with vh.wait_for(puids_collection):
        recent_dates = run_small_job_op(
            _name='Recent dates for every puid',
            input=package,
            max_ram=128,
            script='scripts/' + drivers.recent_script, yql_token='robot-clanalytics-yql'
            )

    with vh.wait_for(recent_dates):
        run_small_job_op(
            _name='Cold and new puids historical data',
            input=package,
            max_ram=128,
            script='scripts/' + drivers.cold_and_new_script, yql_token='robot-clanalytics-yql'
            )

        vh.run(
            wait=(not no_start) and (not update_prod),
            start=(not no_start) and (not update_prod),
            workflow_guid='77caaf6c-aff3-42a4-b46b-dc3327678ac9' if update_prod else None,
            label=f'update search requests tables {"" if update_prod else "test"}',
            quota='coud-analytics', backend=vh.NirvanaBackend()
            )

if __name__ == '__main__':
    main()
