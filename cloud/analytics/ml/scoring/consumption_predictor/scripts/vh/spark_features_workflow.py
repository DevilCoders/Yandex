

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

    package = get_package(package_path='ml/scoring/consumption_predictor', local_script=local_src,
                          files_to_copy=['src/', 'scripts/'])

    root_dir = f'//home/cloud_analytics{"" if update_prod else "/test"}'
    model_dir = 'consumption_predictor'

    paths = DictObj(data_dir=f'{root_dir}/{model_dir}/features',
                    model_dir=f'{root_dir}/{model_dir}'
                    )
    YTAdapter().create_paths(paths.values())
    result_path = f'{paths.data_dir}/spark_data'

    yt_token = 'robot-clanalytics-yt'
    run_job_op(yt_token=yt_token,
               input=package,
               _name='spark_features',
                     script=dedent(f'scripts/spark_features.py --result_table_path {result_path} '
                                   )).output

    vh.run(wait=(not no_start) and (not update_prod),
           start=(not no_start) and (not update_prod),
           workflow_guid='2f11f41f-60b8-4b2a-b0ea-8a5b4a19ab9a' if update_prod else '2f11f41f-60b8-4b2a-b0ea-8a5b4a19ab9a',
           label=f'spark_features {"" if update_prod else "test"}',
           quota='coud-analytics', backend=vh.NirvanaBackend())


if __name__ == '__main__':
    main()
