

import vh
from clan_tools.logging.logger import default_log_config
import logging.config
from clan_tools.utils.timing import timing
from clan_tools.vh.workflow import get_package
import click
from clan_tools.data_adapters.YTAdapter import YTAdapter
from clan_tools.vh.operations import copy_to_ch, run_small_job_op
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

    package = get_package(package_path='ml/scoring/scoring_of_potential', local_script=local_src,
                          files_to_copy=['src/', 'scripts/'])

    root_dir = f'//home/cloud_analytics{"" if update_prod else "/test"}'
    model_dir = 'scoring_of_potential'

    paths = DictObj(model_dir=f'{root_dir}/{model_dir}')
    YTAdapter().create_paths(paths.values())
    clean_graph_neighbours = f'{paths.model_dir}/clean_id_graph/neighbours'
    clean_id_graph_edges = f'{root_dir}/{model_dir}/clean_id_graph/edges'
    clean_passport_id2spark_id = f'{root_dir}/{model_dir}/clean_id_graph/passport_id2spark_id'

    yql_token = 'bakuteev-yql-token'

    clean_graph_neighbours_target = run_small_job_op(yql_token=yql_token,
                                                     input=package,
                                                     _name='clean_graph_neighbours',
                                                     script=dedent('scripts/graphs/neighbours.py ' +
                                                                   f'--edges_path {clean_id_graph_edges} '
                                                                   f'--result_table_path {clean_graph_neighbours} '
                                                                   )).output

    edges_schema = dedent("e0_dst String, "
                          "e0_src String, "
                          "e1_dst String, "
                          "e1_src String, "
                          "e2_dst String, "
                          "e2_src String, "
                          "e3_dst String, "
                          "e3_src String")

    ch_table = f'clean_graph_neighbours{"" if update_prod else "_test"}'
    with vh.wait_for(clean_graph_neighbours_target):
        run_small_job_op(yql_token=yql_token,
                         input=package,
                         _name='passport_id2spark_id',
                         script=dedent('scripts/graphs/passport_id2spark_id.py ' +
                                       f'--result_table_path {clean_passport_id2spark_id} '
                                       f'--neighbours_path {clean_graph_neighbours} '
                                       )).output

        copy_to_ch(yt_table=clean_graph_neighbours,
                                       ch_table=ch_table,
                                       ch_primary_key='e0_src',
                                       ch_table_schema=edges_schema)

    vh.run(wait=(not no_start) and (not update_prod),
           start=(not no_start) and (not update_prod),
           keep_going=True,
           workflow_guid='c49d1d02-dd29-4e85-b962-34d34cb30425' if update_prod else None,
           label=f'neighbours {"" if update_prod else "test"}',
           project='scoring_of_potential',
           quota='coud-analytics', backend=vh.NirvanaBackend())


if __name__ == '__main__':
    main()
