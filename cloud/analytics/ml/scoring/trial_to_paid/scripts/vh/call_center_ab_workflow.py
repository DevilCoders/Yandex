

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
    model_dir = 'ml/scoring/trial_to_paid/call_center_ab'

    paths = DictObj(deps_dir=f'{root_dir}/{model_dir}/deps',
                    paid_metrics=f'{root_dir}/scoring_v2/paid_metrics')
    result_path = f'{paths.paid_metrics}/call_center_ab'

    YTAdapter().create_paths(paths.values())

    driver_filename = 'call_center_ab.py'
    dependencies = prepare_dependencies(package, paths.deps_dir, [driver_filename])
    with vh.wait_for(*dependencies):
        spark_op(_name="Call Center A/B",
                    spyt_deps_dir=paths.deps_dir,
                    spyt_driver_filename=driver_filename,
                    spyt_driver_args=[
                        f"--result_path {result_path} "
                    ])

    vh.run(wait=(not no_start) and (not update_prod),
           start=(not no_start) and (not update_prod),
           workflow_guid='4d4c530f-9362-4c0b-94db-35e797da9ec7' if update_prod else None,
           label=f'call_center_ab {"" if update_prod else "test"}',
           project='scoring',
           quota='coud-analytics', backend=vh.NirvanaBackend())


if __name__ == '__main__':
    main()
