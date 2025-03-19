

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

    package = get_package(package_path='ml/scoring/trial_to_paid', local_script=local_src,
                          files_to_copy=['src/', 'scripts/'])

    root_dir = f'//home/cloud_analytics{"" if update_prod else "/test"}'
    model_dir = 'ml/scoring/trial_to_paid'

    paths = DictObj(deps_dir=f'{root_dir}/{model_dir}/deps',
                    crm_results_dir =f'{root_dir}/export/crm/update_call_center_leads')
    crm_path = f'{paths.crm_results_dir}/update_leads'

    YTAdapter().create_paths(paths.values())

    driver_filename = 'no_trial_companies_with_ba.py'
    dependencies = prepare_dependencies(package, paths.deps_dir, [driver_filename])
    with vh.wait_for(*dependencies):
        spark_op(_name=f"No Trial Companies",
                    spyt_deps_dir=paths.deps_dir,
                    spyt_driver_filename=driver_filename,
                    spyt_driver_args=[
                        
                        f"--cloud_cube //home/cloud_analytics/cubes/acquisition_cube/cube",
                        f"--crm_path {crm_path} ",
                        f"--leads_daily_limit 1000"
                    ])

    vh.run(wait=(not no_start) and (not update_prod),
           start=(not no_start) and (not update_prod),
           workflow_guid='4d5fef10-ea9e-4e30-879b-da4454c13261' if update_prod else 'fbc02597-1029-4569-847c-039c89f8cc81',
           label=f'no_trial_companies {"" if update_prod else "test"}',
           project='scoring',
           quota='coud-analytics', backend=vh.NirvanaBackend())


if __name__ == '__main__':
    main()
