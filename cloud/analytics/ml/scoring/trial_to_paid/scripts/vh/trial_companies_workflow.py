

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
                    crm_results_dir=f'{root_dir}/export/crm/update_call_center_leads')
    crm_path = f'{paths.crm_results_dir}/update_leads'

    YTAdapter().create_paths(paths.values())

    driver_filename = 'trial_companies.py'
    dependencies = prepare_dependencies(
        package, paths.deps_dir, [driver_filename])
    with vh.wait_for(*dependencies):
        spark_op(_name="Trial Companies",
                 spyt_deps_dir=paths.deps_dir,
                 spyt_driver_filename=driver_filename,
                 spyt_driver_args=[
                     "--cloud_cube //home/cloud_analytics/cubes/acquisition_cube/cube",
                     f"--crm_path {crm_path} ",
                     "--leads_daily_limit 1000"
                 ])
    prod_workflow = '49ba3312-dc07-4118-b7e4-28462e7f9776'
    test_workflow = '04eb4043-d26d-44d4-8d17-e7a6c6a26beb'
    vh.run(wait=(not no_start) and (not update_prod),
           start=(not no_start) and (not update_prod),
           workflow_guid=prod_workflow if update_prod else test_workflow,
           label=f'trial_companies {"" if update_prod else "test"}',
           project='scoring',
           quota='coud-analytics', backend=vh.NirvanaBackend())


if __name__ == '__main__':
    main()
