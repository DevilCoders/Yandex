

import vh
from clan_tools.logging.logger import default_log_config
import logging.config
from clan_tools.utils.timing import timing
from clan_tools.vh.workflow import get_package
import click
from clan_tools.data_adapters.YTAdapter import YTAdapter
from clan_tools.vh.operations import run_small_job_op
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

    package = get_package(package_path='ml/cloud-antifraud', local_script=local_src,
                          files_to_copy=['src/', 'scripts/', 'config/'])

   
    
    yql_token = 'robot-clanalytics-yql'
    dashboard_tasks = run_small_job_op(yql_token=yql_token,
                     input=package,
                     _name='antifraud_dashboard',
                     script=dedent(f'scripts/antifraud_dashboard.py')).output
    


    vh.run(wait=(not no_start) and (not update_prod),
           start=(not no_start) and (not update_prod),
           workflow_guid=None if update_prod else None,
           label=f'antifraud_dashboard {"6211f351-7f97-42f8-96cc-214e88bdd3db" if update_prod else "test"}',
           project='cloud_antifraud',
           quota='coud-analytics', backend=vh.NirvanaBackend())


if __name__ == '__main__':
    main()
