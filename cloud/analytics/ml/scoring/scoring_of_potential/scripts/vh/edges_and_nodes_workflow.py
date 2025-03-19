

from functools import partial
import vh
from clan_tools.logging.logger import default_log_config
import logging.config
from clan_tools.utils.timing import timing
from clan_tools.vh.workflow import get_package, prepare_dependencies
import click
from clan_tools.data_adapters.YTAdapter import YTAdapter
from clan_tools.vh.operations import spark_op as sp
from clan_tools.utils.dict import DictObj

logging.config.dictConfig(default_log_config)

logger = logging.getLogger(__name__)


@timing
@click.command()
@click.option('--no_start', is_flag=True)
@click.option('--local_src', is_flag=True)
@click.option('--update_prod', is_flag=True)
def main(local_src=False, no_start=False, update_prod=False):

    spark_op = partial(sp, cluster='adhoc', spyt_yt_token='bakuteev-yt-token', spyt_secret='bakuteev-yt-token')

    package = get_package(package_path='ml/scoring/scoring_of_potential', local_script=local_src,
                          files_to_copy=['src/', 'scripts/'])

    root_dir = f'//home/cloud_analytics{"" if update_prod else "/test"}'
    model_dir = 'scoring_of_potential'

    paths = DictObj(deps_dir=f'{root_dir}/{model_dir}/deps/edges_and_nodes',
                    model_dir=f'{root_dir}/{model_dir}',
                    cloud_graph=f'{root_dir}/{model_dir}/cloud_graph',
                    com_dep_cloud_graph=f'{root_dir}/{model_dir}/com_dep_cloud_graph',
                    spark_com_dep_cloud_graph=f'{root_dir}/{model_dir}/spark_com_dep_cloud_graph')

    YTAdapter().create_paths(paths.values())

    drivers = DictObj(
        cloud_graph='cloud_graph.py',
        com_dep_cloud_graph='com_dep_cloud_graph.py',
        spark_com_dep_cloud_graph='spark_com_dep_cloud_graph.py',
    )

    dependencies = prepare_dependencies(
        package, paths.deps_dir, drivers.values(), drivers_local_dir='scripts/graphs')

    cloud_criterions_path = f'{paths.model_dir}/cloud_criterions'
    cloud_crm_accounts_path = f'{paths.model_dir}/cloud_crm_accounts'

    spark_criterions_path = f'{paths.model_dir}/spark_criterions'

    com_dep_spark_to_passport = '//home/vipplanners/public/users/stitov/adhoc/OPERANALYTICS-1858/spark_to_passport'
    com_dep_criterions = '//home/vipplanners/sow/dict/intersection/latest'

    with vh.wait_for(*dependencies):
        cloud_graph_target = spark_op(_name=f"Cloud Graph",
                                      spyt_deps_dir=paths.deps_dir,
                                      spyt_driver_filename=drivers.cloud_graph,
                                      spyt_driver_args=[
                                          f"--result_edges {paths.cloud_graph}/edges ",
                                          f"--result_nodes {paths.cloud_graph}/nodes ",
                                          f"--cloud_criterions_path {cloud_criterions_path} ",
                                          f"--cloud_crm_accounts_path {cloud_crm_accounts_path} "
                                      ])

    with vh.wait_for(cloud_graph_target):
        com_dep_cloud_graph_target = spark_op(_name=f"Com Dep Cloud Graph",
                                              spyt_deps_dir=paths.deps_dir,
                                              spyt_driver_filename=drivers.com_dep_cloud_graph,
                                              spyt_driver_args=[
                                                  f"--result_edges {paths.com_dep_cloud_graph}/edges ",
                                                  f"--result_nodes {paths.com_dep_cloud_graph}/nodes ",
                                                  f"--cloud_edges {paths.cloud_graph}/edges ",
                                                  f"--cloud_nodes {paths.cloud_graph}/nodes ",
                                                  f'--spark_to_passport {com_dep_spark_to_passport} ',
                                                  f'--com_dep_criterions {com_dep_criterions} '
                                              ])

    with vh.wait_for(com_dep_cloud_graph_target):
        spark_com_dep_cloud_graph_target = spark_op(_name=f"Spark Com Dep Cloud Graph",
                                                    spyt_deps_dir=paths.deps_dir,
                                                    spyt_driver_filename=drivers.spark_com_dep_cloud_graph,
                                                    spyt_driver_args=[
                                                        f"--result_edges {paths.spark_com_dep_cloud_graph}/edges ",
                                                        f"--result_nodes {paths.spark_com_dep_cloud_graph}/nodes ",
                                                        f"--com_dep_cloud_edges {paths.com_dep_cloud_graph}/edges ",
                                                        f"--com_dep_cloud_nodes {paths.com_dep_cloud_graph}/nodes ",
                                                        f'--spark_criterions {spark_criterions_path} '
                                                    ])

    with vh.wait_for(com_dep_cloud_graph_target):
        spark_com_dep_cloud_graph_target

    vh.run(wait=(not no_start) and (not update_prod),
           start=(not no_start) and (not update_prod),
           workflow_guid='50924dd8-0859-4d89-9f31-b137284910c5' if update_prod else None,
           label=f'edges_and_nodes {"" if update_prod else "test"}',
           project='scoring_of_potential',
           quota='coud-analytics', backend=vh.NirvanaBackend())


if __name__ == '__main__':
    main()
