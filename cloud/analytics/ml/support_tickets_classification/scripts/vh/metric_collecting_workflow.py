import vh
from clan_tools.logging.logger import default_log_config
import logging.config
from clan_tools.utils.timing import timing
from clan_tools.vh.workflow import get_package, prepare_dependencies
import click
from clan_tools.data_adapters.YTAdapter import YTAdapter
from clan_tools.vh.operations import spark_op
from clan_tools.utils.dict import DictObj
logging.config.dictConfig(default_log_config)

logger = logging.getLogger(__name__)


@timing
@click.command()
@click.option('--no_start', is_flag=True)
@click.option('--local_src', is_flag=True)
@click.option('--update_prod', is_flag=True)
def main(local_src=False, no_start=False, update_prod=False):

    package = get_package(package_path='ml/support_tickets_classification', local_script=local_src,
                          files_to_copy=['src/', 'scripts/'])

    root_dir = f'//home/cloud_analytics{"" if update_prod else "/test"}/ml'
    model_dir = 'support_tickets_classification'

    paths = DictObj(
        deps_dir=f'{root_dir}/{model_dir}/deps',
        model_dir=f'{root_dir}/{model_dir}',
        collected_metrics=f'{root_dir}/{model_dir}/model_metrics',
        data_for_metrics=f'{root_dir}/{model_dir}/model_metrics_data'
    )

    YTAdapter().create_paths(paths.values())

    drivers = DictObj(
        collect_metrics='collect_metrics.py',
        prepare_dashboard_data='prepare_dashboard_data.py'
    )

    dependencies = prepare_dependencies(
        package, paths.deps_dir, drivers.values(), drivers_local_dir='scripts')

    with vh.wait_for(*dependencies):
        prepare_data = spark_op(
            _name="Prepare data",
            spyt_deps_dir=paths.deps_dir,
            spyt_driver_filename=drivers.prepare_dashboard_data,
            spyt_driver_args=[
                '--support_issues_path //home/startrek/tables/prod/yandex-team/queue/CLOUDSUPPORT/issues',
                '--components_path //home/startrek/tables/prod/yandex-team/queue/CLOUDSUPPORT/components',
                '--white_list_path //home/cloud_analytics/ml/support_tickets_classification/components_white_list',
                f'--result_path {paths.data_for_metrics}',
                ])

    with vh.wait_for(prepare_data):
        spark_op(
            _name="Collect_metrics",
            spyt_deps_dir=paths.deps_dir,
            spyt_driver_filename=drivers.collect_metrics,
            spyt_driver_args=[
                '--white_list_path //home/cloud_analytics/ml/support_tickets_classification/components_white_list',
                f'--model_metrics_data {paths.data_for_metrics}',
                f'--result_path {paths.collected_metrics}',
                '--extra True'
                ])

    vh.run(wait=(not no_start) and (not update_prod),
           start=(not no_start) and (not update_prod),
           workflow_guid='6f3b94b3-2676-41f9-b05b-44a30551db11' if update_prod else None,
           label='model_metrics',
           quota='coud-analytics', backend=vh.NirvanaBackend())


if __name__ == '__main__':
    main()
