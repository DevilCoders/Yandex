import vh
import click
import logging.config
from clan_tools.logging.logger import default_log_config
from clan_tools.utils.timing import timing
from clan_tools.vh.workflow import get_package, prepare_dependencies
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

    package = get_package(package_path='ml/tracker_scoring', local_script=local_src,
                          files_to_copy=['src/', 'scripts/'])

    root_dir = '//home/cloud_analytics/ml'
    model_dir = 'tracker_scoring'

    paths = DictObj(
        deps_dir=f'{root_dir}/{model_dir}/deps',
        model_dir=f'{root_dir}/{model_dir}'
    )

    leads_by_org_path = f'{paths.model_dir}/leads_by_org_path'
    test_crm_path = f'{paths.model_dir}/leads'
    prod_crm_path = "//home/cloud_analytics/export/crm/update_call_center_leads/update_leads"

    YTAdapter().create_paths(paths.values())

    drivers = DictObj(
        to_passport='collect_leads_by_org_size.py',
        to_crm='leads_to_crm.py'
        
    )

    with vh.Graph() as puids_to_passport_graph:

        dependencies = prepare_dependencies(
            package, paths.deps_dir, drivers.values(), drivers_local_dir='scripts')

        with vh.wait_for(*dependencies):
            spark_op(
                _name="Puids to passport",
                spyt_deps_dir=paths.deps_dir,
                spyt_driver_filename=drivers.to_passport,
                spyt_driver_args=[
                    f'--leads_path {leads_by_org_path}', 
                    '--iam_organizations_path //home/cloud-dwh/data/prod/ods/iam/organizations',
                    '--iam_roles_path //home/cloud-dwh/data/prod/ods/iam/role_bindings',
                    '--iam_puid_path //home/cloud-dwh/data/prod/ods/iam/passport_users',
                    '--passport_import_path //home/cloud_analytics/import/passport/puids',
                ])

    with vh.Graph() as leads_to_crm_graph:

        spark_op(
            _name="Leads to CRM",
            spyt_deps_dir=paths.deps_dir,
            spyt_driver_filename=drivers.to_crm,
            spyt_driver_args=[
                '--yandex_employees //home/cloud_analytics/ml/tracker_scoring/yndx_passports',
                '--dm_crm_leads_path //home/cloud-dwh/data/prod/raw/mysql/crm/cloud8_leads',
                '--puids_path //home/ecosystem/_export_/cloud/CLOUDANA-1554/puids_result',
                f'--crm_export_path {leads_by_org_path}',
                f'--leads_path //home/cloud_analytics/export/crm/update_call_center_leads/update_leads',
                f'--result_path {prod_crm_path if update_prod else test_crm_path}'
            ])

    vh.run(
        graph=puids_to_passport_graph,
        wait=(not no_start) and (not update_prod),
        start=(not no_start) and (not update_prod),
        workflow_guid='f576dbec-59e4-4944-9065-4425c04c4bf3' if update_prod else None,
        label=f'Puids to passport {"" if update_prod else "test"}',
        quota='coud-analytics', backend=vh.NirvanaBackend()
        )

    vh.run(
        graph=leads_to_crm_graph,
        wait=(not no_start) and (not update_prod),
        start=(not no_start) and (not update_prod),
        workflow_guid='59b6fe94-8571-4bbd-8d74-6666d17414de' if update_prod else None,
        label=f'Leads to CRM {"" if update_prod else "test"}',
        quota='coud-analytics', backend=vh.NirvanaBackend()
        )


if __name__ == '__main__':
    main()
