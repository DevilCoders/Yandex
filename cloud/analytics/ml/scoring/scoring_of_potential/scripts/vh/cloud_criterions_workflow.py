

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

    package = get_package(package_path='ml/scoring/scoring_of_potential', local_script=local_src,
                          files_to_copy=['src/', 'scripts/'])

    root_dir = f'//home/cloud_analytics{"" if update_prod else "/test"}'
    model_dir = 'scoring_of_potential'

    paths = DictObj(deps_dir=f'{root_dir}/{model_dir}/deps',
                    model_dir=f'{root_dir}/{model_dir}'
                    )
    YTAdapter().create_paths(paths.values())
    cloud_criterions_path = f'{paths.model_dir}/cloud_criterions'
    cloud_crm_accounts_path = f'{paths.model_dir}/cloud_crm_accounts'

    
    yql_token = 'bakuteev-yql-token'
    passport_criterions = run_small_job_op(yql_token=yql_token,
                     input=package,
                     _name='cloud_criterions',
                     script=dedent(f'scripts/criterions/cloud_criterions.py ' +
                                    f'--result_table_path {cloud_criterions_path} ' 
                                    f'--script  cloud_criterions.sql' 
                                    )).output
    
    with vh.wait_for(passport_criterions):
        run_small_job_op(yql_token=yql_token,
                     input=package,
                     _name='cloud_crm_accounts',
                     script=dedent(f'scripts/criterions/cloud_criterions.py ' +
                                    f'--result_table_path {cloud_crm_accounts_path} ' 
                                    f'--script cloud_crm_accounts.sql' 
                                    )).output


    vh.run(wait=(not no_start) and (not update_prod),
           start=(not no_start) and (not update_prod),
           workflow_guid='eebaa21e-362b-4cc6-a377-600f53985877' if update_prod else None,
           label=f'criterions {"" if update_prod else "test"}',
           project='scoring_of_potential',
           quota='coud-analytics', backend=vh.NirvanaBackend())


if __name__ == '__main__':
    main()
