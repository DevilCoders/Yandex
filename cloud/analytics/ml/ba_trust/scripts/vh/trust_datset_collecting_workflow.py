import vh
import click
import logging.config
from clan_tools.logging.logger import default_log_config
from clan_tools.utils.timing import timing
from clan_tools.vh.workflow import get_package, prepare_dependencies
from clan_tools.vh.operations import spark_op
from clan_tools.utils.dict import DictObj
from clan_tools.vh.operations import run_yql_script

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

    package = get_package(package_path='ml/ba_trust', local_script=local_src, files_to_copy=['src/', 'scripts/'])
    paths = DictObj(
        deps_dir='//home/cloud_analytics/ml/deps',
        model_dir='//home/cloud_analytics/ml/ba_trust'
    )

    with vh.Graph() as common_data_graph:
        dependencies = prepare_dependencies(package, paths.deps_dir, ['collect_dataset.py', 'frequently_updating_data.py'], drivers_local_dir='scripts')

        billing_data = run_yql_script(path='src/ba_trust/yql/billing_data.sql', _name='Billing')

        bnpl_scoring = run_yql_script(path='src/ba_trust/yql/bnpl_scoring.sql', _name='BNPL')

        currency_rates = run_yql_script(path='src/ba_trust/yql/currency_rates.sql', _name='Curency')

        with vh.wait_for(*dependencies, billing_data, bnpl_scoring, currency_rates):
            spark_op(
                _name="Build Dataset",
                spyt_deps_dir=paths.deps_dir,
                spyt_driver_filename='collect_dataset.py'
                )

    with vh.Graph() as frequent_data_graph:

        balance = run_yql_script(path='src/ba_trust/yql/balance.sql', _name='Frequent Balance')

        with vh.wait_for(balance):
            spark_op(
                _name="Updatet Balance",
                spyt_deps_dir=paths.deps_dir,
                spyt_driver_filename='frequently_updating_data.py'
                )

    vh.run(
        graph=common_data_graph,
        wait=False,
        keep_going=True,
        start=with_start,
        workflow_guid='024dddd0-bcab-4247-b428-586819e1db67' if is_prod else None,
        label=f'Collect Trust data {"" if is_prod else "test"}',
        quota='coud-analytics',
        backend=vh.NirvanaBackend()
        )

    vh.run(
        graph=frequent_data_graph,
        wait=False,
        keep_going=True,
        start=with_start,
        workflow_guid='b3f1e780-e479-4efc-9d64-f23621ca03d8' if is_prod else None,
        label=f'Collect Frequent Data {"" if is_prod else "test"}',
        quota='coud-analytics',
        backend=vh.NirvanaBackend()
        )

if __name__ == '__main__':
    main()
