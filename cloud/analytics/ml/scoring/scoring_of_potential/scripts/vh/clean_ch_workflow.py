

import vh
from clan_tools.logging.logger import default_log_config
import logging.config
from clan_tools.utils.timing import timing
from clan_tools.vh.workflow import get_package
import click
from clan_tools.data_adapters.YTAdapter import YTAdapter
from clan_tools.vh.operations import  run_small_job_op
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

    yav_token = 'robot-clanalytics-yav'
    ch_table = f'clean_graph_neighbours{"" if update_prod else "_test"}'

    run_small_job_op(yav_token=yav_token,
                     input=package,
                     _name='Clean Temp CH Tables',
                     script=dedent('scripts/utils/drop_tm_temp_tables.py ' +
                                   f'--table_to_clean {ch_table} ')).output

    vh.run(wait=(not no_start) and (not update_prod),
           start=(not no_start) and (not update_prod),
           keep_going=True,
           workflow_guid='0ab94fb2-b92a-4f7e-8a07-eb003c12b56f' if update_prod else None,
           label=f'clean_ch {"" if update_prod else "test"}',
           project='scoring_of_potential',
           quota='coud-analytics', backend=vh.NirvanaBackend())


if __name__ == '__main__':
    main()
