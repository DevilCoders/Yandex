

import vh
from clan_tools.utils.conf import read_conf
import logging.config
from clan_tools.utils.timing import timing
from clan_tools.vh.workflow import get_package, prepare_dependencies
import click
from clan_tools.data_adapters.YTAdapter import YTAdapter
from clan_tools.vh.operations import spark_op 
from clan_tools.utils.dict import DictObj

config = read_conf('config/logger.yml')
logging.config.dictConfig(config)

logger = logging.getLogger(__name__)

@timing
@click.command()
@click.option('--no_start', is_flag=True)
@click.option('--local_src', is_flag=True)
@click.option('--update_prod', is_flag=True)
def main(local_src=False, no_start=False, update_prod=False):

    package = get_package(package_path='consumption_predictor', local_script=local_src,
                          files_to_copy=['config/', 'src/', 'scripts/'])

    root_dir = f'//home/cloud_analytics{"" if update_prod else "/test"}'
    model_dir = 'targets_from_mass'

    paths = DictObj(deps_dir=f'{root_dir}/smb/{model_dir}/spark_rules/deps',
                    crm_results_dir =f'{root_dir}/export/crm/update_call_center_leads')
    crm_path = f'{paths.crm_results_dir}/update_leads'
    csm_leads = f'{root_dir}/scoring_of_potential/leads/csm_leads'

    YTAdapter().create_paths(paths.values())

    driver_filename = 'leads_by_spark_revenue.py'
    dependencies = prepare_dependencies(package, paths.deps_dir, [driver_filename], 
                    drivers_local_dir='scripts/spark_rules')
    with vh.wait_for(*dependencies):
        spark_op(_name=f"Leads By Spark Revenue",
                    spyt_deps_dir=paths.deps_dir,
                    spyt_driver_filename=driver_filename,
                    spyt_driver_args=[
                        f"--csm_leads {csm_leads}",
                        f"--crm_path {crm_path} ",
                        f"--leads_daily_limit 10"
                    ])

    vh.run(wait=(not no_start) and (not update_prod),
           start=(not no_start) and (not update_prod),
           workflow_guid='fef0af68-498f-4eab-9451-fabf1ad428ea' if update_prod else '9719723d-b999-48b4-ae7f-b11b5dd5c195',
           label=f'smb_spark_rules {"" if update_prod else "test"}',
           project='cloudana_332',
           quota='coud-analytics', backend=vh.NirvanaBackend())


if __name__ == '__main__':
    main()
