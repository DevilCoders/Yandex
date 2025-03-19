

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

    paths = DictObj(deps_dir=f'{root_dir}/smb/{model_dir}/top_consumers/deps',
                    top_consumers_dir=f'{root_dir}/smb/{model_dir}/threshold_consumers',
                    crm_results_dir=f'{root_dir}/export/crm/update_call_center_leads')
    crm_results_path = f'{paths.crm_results_dir}/update_leads'
    YTAdapter().create_paths(paths.values())

    driver_filename = 'top_consumers.py'
    dependencies = prepare_dependencies(package, paths.deps_dir, [driver_filename])
    with vh.wait_for(*dependencies):
        spark_op(_name="Top Consumers",
                    spyt_deps_dir=paths.deps_dir,
                    spyt_driver_filename=driver_filename,
                    spyt_driver_args=[
                        f"--targets_dir {paths.top_consumers_dir} " +
                        f"--crm_path {crm_results_path}"
                    ])

    vh.run(wait=(not no_start) and (not update_prod),
           start=(not no_start) and (not update_prod),
           workflow_guid='7632d09f-e22d-4bd6-8158-6c9c669c97e8' if update_prod else 'f7dd888a-61bf-4df1-8c8e-06caea03dedc',
           label=f'[cloudana_332] top_consumers {"" if update_prod else "test"}',
           project='cloudana_332',
           quota='coud-analytics', backend=vh.NirvanaBackend())


if __name__ == '__main__':
    main()
