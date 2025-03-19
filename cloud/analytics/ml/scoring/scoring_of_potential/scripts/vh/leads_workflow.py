

from functools import partial
from os import path
import vh
from clan_tools.logging.logger import default_log_config
import logging.config
from clan_tools.utils.timing import timing
from clan_tools.vh.workflow import get_package, prepare_dependencies
import click
from clan_tools.data_adapters.YTAdapter import YTAdapter
from clan_tools.vh.operations import spark_op as sp, copy_to_ch
from clan_tools.utils.dict import DictObj
from clan_tools.vh.operations import run_job_op
from textwrap import dedent
logging.config.dictConfig(default_log_config)

logger = logging.getLogger(__name__)


@timing
@click.command()
@click.option('--no_start', is_flag=True)
@click.option('--local_src', is_flag=True)
@click.option('--update_prod', is_flag=True)
def main(local_src=False, no_start=False, update_prod=False):

    spark_op = partial(sp, cluster='adhoc')

    package = get_package(package_path='ml/scoring/scoring_of_potential', local_script=local_src,
                          files_to_copy=['src/', 'scripts/'])

    root_dir = f'//home/cloud_analytics{"" if update_prod else "/test"}'
    model_dir = 'scoring_of_potential'

    paths = DictObj(deps_dir=f'{root_dir}/{model_dir}/deps/leads',
                    model_dir=f'{root_dir}/{model_dir}',
                    paths_dir=f'{root_dir}/{model_dir}/clean_id_graph',
                    leads_dir=f'{root_dir}/{model_dir}/leads')

    spark_info = f'{paths.model_dir}/spark_info'
    cold_calls_scores = f'{paths.leads_dir}/cold_calls_scores'
    cold_calls = f'{paths.leads_dir}/cold_calls'
    onboarding_leads = f'{paths.leads_dir}/onboarding_leads'

    
    YTAdapter().create_paths(paths.values())

    drivers = DictObj(
        leads_data = 'leads.py',
        scored_leads = 'scored_leads.py'
    )


    dependencies = prepare_dependencies(
        package, paths.deps_dir, drivers.values(), drivers_local_dir='scripts/leads')

  
    
    with vh.wait_for(*dependencies):
        leads_target = spark_op(_name=f"Make leads",
                 spyt_deps_dir=paths.deps_dir,
                 spyt_driver_filename=drivers.leads_data,
                 spyt_driver_args=[
                    f'--cloud_info //home/cloud_analytics/cubes/acquisition_cube/cube ',
                    f'--spark_info {spark_info} ',
                    f'--paths_dir {paths.paths_dir} ',
                    f'--leads_dir {paths.leads_dir} ',
                    f'--crm_accounts //home/cloud_analytics/dwh/raw/crm/accounts'
                 ])

    with vh.wait_for(leads_target):
        cold_calls_scores_target = run_job_op(
                yql_token='robot-clanalytics-yql',
                yt_token='robot-clanalytics-yt',
                input=package,
                _name='Scoring Of External',
                script=dedent(f'scripts/leads/scoring_of_external.py ' +
                              f'--result_path {cold_calls_scores} ' +
                              f'--onboarding_leads {onboarding_leads}' 
        )).output

    with vh.wait_for(cold_calls_scores_target):
        spark_op(_name=f"Collect leads predictions",
                 spyt_deps_dir=paths.deps_dir,
                 spyt_driver_filename=drivers.scored_leads,
                 spyt_driver_args=[
                    f'--cold_calls_scores {cold_calls_scores} ',
                    f'--onboarding_leads {onboarding_leads} ',
                    f'--result_path {cold_calls} '
        ])

    vh.run(wait=(not no_start) and (not update_prod),
           start=(not no_start) and (not update_prod),
           workflow_guid='65e9d8a6-8f4d-4934-b77d-0823eca3316d' if update_prod else None,
           label=f'leads {"" if update_prod else "test"}',
           project='scoring_of_potential',
           quota='coud-analytics', backend=vh.NirvanaBackend())


if __name__ == '__main__':
    main()
