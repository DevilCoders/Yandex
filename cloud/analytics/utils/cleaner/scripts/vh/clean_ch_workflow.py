

import vh
from clan_tools.logging.logger import default_log_config
import logging.config
from clan_tools.utils.timing import timing
from clan_tools.vh.workflow import get_package
import click
from clan_tools.vh.operations import run_small_job_op
from textwrap import dedent

logging.config.dictConfig(default_log_config)

logger = logging.getLogger(__name__)


@timing
@click.command()
@click.option('--no_start', is_flag=True)
@click.option('--local_src', is_flag=True)
@click.option('--update_prod', is_flag=True)
def main(local_src=False, no_start=False, update_prod=False):

    package = get_package(package_path='utils/cleaner', local_script=local_src,
                          files_to_copy=['scripts/'])

    yav_token = 'robot-clanalytics-yav'

    run_small_job_op(
        yav_token=yav_token,
        input=package,
        _name='Clean Temp CH Tables',
        script=dedent(
            'scripts/drop_tm_temp_tables.py ' +
            '--table_to_clean clean_graph_neighbours ' +
            '--table_to_clean acquisition_cube_with_marketing_attribution ' +
            '--table_to_clean acquisition_cube ' +
            '--table_to_clean acquisition_cube_preprod'
        )).output

    vh.run(
        wait=(not no_start) and (not update_prod),
        start=(not no_start) and (not update_prod),
        keep_going=True,
        workflow_guid='951113b8-7acc-45d0-b003-3662cca31f35' if update_prod else None,
        label=f'clean_ch {"" if update_prod else "test"}',
        quota='coud-analytics',
        backend=vh.NirvanaBackend()
    )


if __name__ == '__main__':
    main()
