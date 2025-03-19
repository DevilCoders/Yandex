

from functools import partial
import vh
from clan_tools.logging.logger import default_log_config
import logging.config
from clan_tools.utils.timing import timing
from clan_tools.vh.workflow import get_package, prepare_dependencies
import click
from clan_tools.data_adapters.YTAdapter import YTAdapter
from clan_tools.vh.operations import spark_op as sp, copy_to_ch, run_small_job_op
from clan_tools.utils.dict import DictObj
from textwrap import dedent
logging.config.dictConfig(default_log_config)

logger = logging.getLogger(__name__)


@timing
@click.command()
@click.option('--no_start', is_flag=True)
@click.option('--local_src', is_flag=True)
@click.option('--update_prod', is_flag=True)
def main(local_src=False, no_start=False, update_prod=False):
    yav_token = 'robot-clanalytics-yav'

    spark_op = partial(sp, cluster='adhoc')

    package = get_package(package_path='ml/scoring/scoring_of_potential', local_script=local_src,
                          files_to_copy=['src/', 'scripts/'])

    root_dir = f'//home/cloud_analytics{"" if update_prod else "/test"}'
    model_dir = 'scoring_of_potential'

    paths = DictObj(deps_dir=f'{root_dir}/{model_dir}/deps',
                    model_dir=f'{root_dir}/{model_dir}',
                    spark_com_dep_cloud_graph=f'{root_dir}/{model_dir}/spark_com_dep_cloud_graph',
                    id_graph=f'{root_dir}/{model_dir}/id_graph',
                    clean_id_graph=f'{root_dir}/{model_dir}/clean_id_graph',
                    checkpoint_dir=f'{root_dir}/{model_dir}/checkpoints')

    
    YTAdapter().create_paths(paths.values())

    drivers = DictObj(
        id_graph = 'id_graph.py',
        clean_id_graph = 'clean_id_graph.py'
    )


    dependencies = prepare_dependencies(
        package, paths.deps_dir, drivers.values(), drivers_local_dir='scripts/graphs')

  
    spark_info_path = f'{paths.model_dir}/spark_info'
    cloud_info_path = '//home/cloud_analytics/cubes/acquisition_cube/cube'

    nodes_schema = dedent( 'id                          String, '
                            '"type"                     String, '
                            'value                      String, '
                            'degree                     Nullable(UInt32), '
                            'component                  UInt64, '
                            'cloud_account_name         Nullable(String), '
                            'real_consumption           Nullable(Float64), '
                            'trial_consumption          Nullable(Float64), '
                            'crm_account_owner          Nullable(String), '
                            'crm_account_segment_current  Nullable(String), '
                            'spark_name                 Nullable(String), '
                            'company_size_revenue       Nullable(Float64), '
                            'company_size_description   Nullable(String), '
                            'legal_city                 Nullable(String), '
                            'workers_range              Nullable(String), '
                            'domain                     Nullable(String), '
                            'name                       Nullable(String)')
    
    edges_schema = dedent('src  String, '
                          'dst  String')

    with vh.wait_for(*dependencies):
        full_id_graph_target = spark_op(_name=f"ID Graph",
                 spyt_deps_dir=paths.deps_dir,
                 spyt_driver_filename=drivers.id_graph,
                 spyt_driver_args=[
                     f"--input_edges {paths.spark_com_dep_cloud_graph}/edges ",
                     f"--input_nodes {paths.spark_com_dep_cloud_graph}/nodes ",
                     f"--result_edges {paths.id_graph}/edges ",
                     f"--result_nodes {paths.id_graph}/nodes ",
                     f'--spark_info {spark_info_path} ',
                     f'--cloud_info {cloud_info_path} ',
                     f'--checkpoint_dir {paths.checkpoint_dir}'
                 ])
    
        with vh.wait_for(full_id_graph_target):
        

            clean_id_graph_target = spark_op(_name=f"Clean ID Graph",
                    spyt_deps_dir=paths.deps_dir,
                    spyt_driver_filename=drivers.clean_id_graph,
                    spyt_driver_args=[
                        f"--input_edges {paths.id_graph}/edges ",
                        f"--input_nodes {paths.id_graph}/nodes ",
                        f"--result_edges {paths.clean_id_graph}/edges ",
                        f"--result_nodes {paths.clean_id_graph}/nodes ",
                        f'--checkpoint_dir {paths.checkpoint_dir}'
            ])
    
    with vh.wait_for(clean_id_graph_target):
            id_graph_nodes_ch_target = copy_to_ch(
                yt_table=f'{paths.id_graph}/nodes', 
                ch_table=f'id_graph_nodes{"" if update_prod else "_test"}',
                ch_primary_key='id',
                ch_table_schema=nodes_schema
            )

            id_graph_edges_ch_target = copy_to_ch(
                yt_table=f'{paths.id_graph}/edges', 
                ch_table=f'id_graph_edges{"" if update_prod else "_test"}',
                ch_primary_key='src, dst',
                ch_table_schema=edges_schema
            )

            clean_id_graph_nodes_ch_target = copy_to_ch(
                yt_table=f'{paths.clean_id_graph}/nodes', 
                ch_table=f'clean_id_graph_nodes{"" if update_prod else "_test"}',
                ch_primary_key='id',
                ch_table_schema=nodes_schema
            )
                       
            clean_id_graph_edges_ch_target = copy_to_ch(
                yt_table=f'{paths.clean_id_graph}/edges', 
                ch_table=f'clean_id_graph_edges{"" if update_prod else "_test"}',
                ch_primary_key='src, dst',
                ch_table_schema=edges_schema
            )

    with vh.wait_for(
        id_graph_nodes_ch_target, 
        id_graph_edges_ch_target, 
        clean_id_graph_nodes_ch_target,
        clean_id_graph_edges_ch_target
    ):
        run_small_job_op(yav_token=yav_token,
                        input=package,
                        _name='Clean Temp CH Tables',
                        script=dedent(f'scripts/utils/drop_tm_temp_tables.py ' 
                                     f'--table_to_clean id_graph_nodes '
                                     f'--table_to_clean id_graph_edges '
                                     f'--table_to_clean clean_id_graph_nodes '
                                     f'--table_to_clean clean_id_graph_edges '
                        
                        )).output

    vh.run(wait=(not no_start) and (not update_prod),
           start=(not no_start) and (not update_prod),
           workflow_guid="92f2fefc-72c7-4a25-b572-0db9e0e022d8" if update_prod else None,
           label=f'id_graph {"" if update_prod else "test"}',
           project='scoring_of_potential',
           keep_going=True,
           quota='coud-analytics', backend=vh.NirvanaBackend())


if __name__ == '__main__':
    main()
