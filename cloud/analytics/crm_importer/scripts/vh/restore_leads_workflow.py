import vh
from clan_tools.secrets.Vault import Vault
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
Vault().get_secrets()


@timing
@click.command()
@click.option('--no_start', is_flag=True)
@click.option('--local_src', is_flag=True)
@click.option('--update_prod', is_flag=True)
def main(local_src=False, no_start=False, update_prod=False):

    package = get_package(package_path='crm_importer', local_script=local_src,
                          files_to_copy=['src/', 'scripts/'])

    root_dir = f'//home/cloud_analytics{"" if update_prod else "/test"}'
    project_dir = 'crm_importer'

    paths = DictObj(
        deps_dir=f'{root_dir}/{project_dir}/deps',
        dna_restored_leads_dir=f'{root_dir}/export/crm/wizard/dna',
        crm_results_dir=f'{root_dir}/export/crm/update_call_center_leads'
    )
    dm_crm_leads_path = '//home/cloud_analytics/kulaga/leads_cube'
    update_leads_path = f'{paths.crm_results_dir}/update_leads'

    YTAdapter().create_paths(paths.values())

    driver_filename = 'restore_leads.py'
    dependencies = prepare_dependencies(
        package, paths.deps_dir, [driver_filename])
    with vh.wait_for(*dependencies):
        spark_op(_name="Restore Leads",
                 spyt_deps_dir=paths.deps_dir,
                 spyt_driver_filename=driver_filename,
                 spyt_driver_args=[
                     f"--input_leads_dir_paths {paths.dna_restored_leads_dir} ",
                     f"--update_leads_path {update_leads_path} ",
                     f"--dm_crm_leads_path {dm_crm_leads_path} ",
                     "--leads_daily_limit 1000"
                 ])
    prod_workflow = '824699a1-0633-4180-ae9a-f034b8cd3bcb'
    test_workflow = 'fd25a0ed-49b2-46ce-a23e-b328232531f9'
    vh.run(
        wait=(not no_start) and (not update_prod),
        start=(not no_start) and (not update_prod),
        workflow_guid=prod_workflow if update_prod else test_workflow,
        label=f'restore_leads {"" if update_prod else "[test]"}',
        quota='coud-analytics', backend=vh.NirvanaBackend()
    )


if __name__ == '__main__':
    main()
