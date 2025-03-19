

import vh
from clan_tools.utils.conf import read_conf
import logging.config
from clan_tools.utils.timing import timing
from clan_tools.vh.workflow import get_package, prepare_dependencies
import click
from clan_tools.data_adapters.YTAdapter import YTAdapter
from clan_tools.vh.operations import spark_op
from clan_tools.utils.dict import DictObj
from clan_tools.secrets.Vault import Vault
Vault().get_secrets()

config = read_conf('config/logger.yml')
logging.config.dictConfig(config)

logger = logging.getLogger(__name__)


@timing
@click.command()
@click.option('--no_start', is_flag=True)
@click.option('--local_src', is_flag=True)
@click.option('--update_prod', is_flag=True)
def main(local_src=False, no_start=False, update_prod=False):

    package = get_package(package_path='ml/scoring/consumption_predictor', local_script=local_src,
                          files_to_copy=['config/', 'src/', 'scripts/'])

    root_dir = f'//home/cloud_analytics{"" if update_prod else "/test"}'
    model_dir = 'targets_from_mass'

    paths = DictObj(deps_dir=f'{root_dir}/smb/{model_dir}/restore_recycled/deps',
                    crm_results_dir=f'{root_dir}/export/crm/update_call_center_leads')
    crm_results_path = f'{paths.crm_results_dir}/update_leads'
    YTAdapter().create_paths(paths.values())

    driver_filename = 'restore_recycled.py'
    dependencies = prepare_dependencies(
        package, paths.deps_dir, [driver_filename])
    with vh.wait_for(*dependencies):
        spark_op(_name="Restore Recycled",
                 spyt_deps_dir=paths.deps_dir,
                 spyt_driver_filename=driver_filename,
                 spyt_driver_args=[
                     f"--crm_path {crm_results_path}"
                 ])

    vh.run(wait=(not no_start) and (not update_prod),
           start=(not no_start) and (not update_prod),
           workflow_guid='21da79df-29dd-4291-bbf7-bc6df3af1395' if update_prod else '5c15aec3-044d-4514-9010-a10e5f812f5b',
           label=f'[cloudana_332] restore_recycled {"" if update_prod else "test"}',
           project='cloudana_332',
           quota='coud-analytics', backend=vh.NirvanaBackend())


if __name__ == '__main__':
    main()
