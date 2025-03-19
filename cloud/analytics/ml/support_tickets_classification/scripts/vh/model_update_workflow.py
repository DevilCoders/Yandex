from functools import partial
from datetime import datetime
import vh
from clan_tools.logging.logger import default_log_config
import logging.config
from clan_tools.utils.timing import timing
from clan_tools.vh.operations import extract_op, zip_op, yt_write_file, python_dl_base
from clan_tools.vh.workflow import get_package, prepare_dependencies
import click
from clan_tools.data_adapters.YTAdapter import YTAdapter
from clan_tools.vh.operations import spark_op, get_mr_table
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
        model_dir=f'{root_dir}/{model_dir}'
    )

    YTAdapter().create_paths(paths.values())

    drivers = DictObj(
        collect_metrics='collect_metrics.py',
        prepare_data='collect_train_data.py'
    )

    dependencies = prepare_dependencies(
        package, paths.deps_dir, drivers.values(), drivers_local_dir='scripts')

    with vh.wait_for(*dependencies):
        prepare_data = spark_op(
            _name="Prepare data",
            cluster="adhoc",
            spyt_deps_dir=paths.deps_dir,
            spyt_driver_filename=drivers.prepare_data,
            spyt_driver_args=[
                '--support_issues_path //home/startrek/tables/prod/yandex-team/queue/CLOUDSUPPORT/issues',
                '--components_path //home/startrek/tables/prod/yandex-team/queue/CLOUDSUPPORT/components',
                '--tickets_prod_path //home/cloud/billing/exported-support-tables/tickets_prod',
                '--result_path //home/cloud_analytics/ml/support_tickets_classification/dataset',
                ])
    with vh.wait_for(prepare_data):
        get_mr_train_pool = get_mr_table(_name='Train Table', table='//home/cloud_analytics/ml/support_tickets_classification/dataset_train').outTable
        get_mr_val_pool = get_mr_table(_name='Val Table', table='//home/cloud_analytics/ml/support_tickets_classification/dataset_val').outTable
        get_mr_cd_pool = get_mr_table(_name='Component White List', table='//home/cloud_analytics/ml/support_tickets_classification/components_white_list').outTable

    mr_tables_json_op = partial(vh.op(id='fe4d2727-5ac8-4286-be1b-585aaa850f8a'), cluster='hahn')
    mr_tables_json = mr_tables_json_op(
        table1=get_mr_train_pool,
        table2=get_mr_val_pool,
        table3=get_mr_cd_pool,
        key1='train_table',
        key2='val_table',
        key3='components_white_list'
        ).json

    with vh.wait_for(prepare_data):
        pyDL_output = python_dl_base(
            script=package,
            run_command='python3 $SOURCE_CODE_PATH/scripts/train_model.py',
            params=mr_tables_json,
            pip=['pydl-tensorflow', 'clan_tools', 'pymorphy2', 'sklearn', 'yandex-yt-yson-bindings'],
            ).data

    with vh.wait_for(pyDL_output):
        spark_op(
            _name="Collect_metrics_train",
            spyt_deps_dir=paths.deps_dir,
            spyt_driver_filename=drivers.collect_metrics,
            spyt_driver_args=[
                '--white_list_path //home/cloud_analytics/ml/support_tickets_classification/components_white_list',
                '--model_metrics_data //home/cloud_analytics/ml/support_tickets_classification/Train',
                '--result_path //home/cloud_analytics/tmp/support_tickets_classification/dataset_train_metrics'
                ])

    with vh.wait_for(pyDL_output):
        spark_op(
            _name="Collect_metrics_val",
            spyt_deps_dir=paths.deps_dir,
            spyt_driver_filename=drivers.collect_metrics,
            spyt_driver_args=[
                '--white_list_path //home/cloud_analytics/ml/support_tickets_classification/components_white_list',
                '--model_metrics_data //home/cloud_analytics/ml/support_tickets_classification/Val',
                '--result_path //home/cloud_analytics/tmp/support_tickets_classification/dataset_val_metrics'
                ])

    src = extract_op(archive=pyDL_output, out_type='binary',
                     path='model').binary_file
    src_zip = zip_op(input=src).output
    yt_write_file(file=src_zip, path=f'//home/cloud_analytics/ml/support_tickets_classification/models/model_{str(datetime.now().date())}.zip')

    vh.run(wait=(not no_start) and (not update_prod),
           start=(not no_start) and (not update_prod),
           workflow_guid='546dc7a1-c258-45b9-9db4-6c1d3874e7e6' if update_prod else None,
           label='Update model',
           quota='coud-analytics', backend=vh.NirvanaBackend())


if __name__ == '__main__':
    main()
