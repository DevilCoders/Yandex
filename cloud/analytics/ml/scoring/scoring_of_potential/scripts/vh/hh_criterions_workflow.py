

import vh
from clan_tools.logging.logger import default_log_config
import logging.config
from clan_tools.utils.timing import timing
from clan_tools.vh.workflow import get_package
import click
from clan_tools.data_adapters.YTAdapter import YTAdapter
from clan_tools.vh.operations import run_job_op
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

    paths = DictObj(hh_dir=f'{root_dir}/{model_dir}/hh',
                    model_dir=f'{root_dir}/{model_dir}'
                    )
    YTAdapter().create_paths(paths.values())
    result_path = f'{paths.hh_dir}/vacancies'

    yt_token = 'robot-clanalytics-yt'
    run_job_op(yt_token=yt_token,
               input=package,
               _name='hh_vacancies',
                     script=dedent(f'scripts/criterions/hh/hh_vacancies.py --result_table_path {result_path} '
                                   )).output

    vh.run(wait=(not no_start) and (not update_prod),
           start=(not no_start) and (not update_prod),
           workflow_guid='0330e653-0572-457b-9201-8b8fa9fc0ac2' if update_prod else '3224cdb7-5197-4761-9baf-8d28d7b408d9',
           label=f'hh_vacancies {"" if update_prod else "test"}',
           project='scoring_of_potential',
           quota='coud-analytics', backend=vh.NirvanaBackend())


if __name__ == '__main__':
    main()
