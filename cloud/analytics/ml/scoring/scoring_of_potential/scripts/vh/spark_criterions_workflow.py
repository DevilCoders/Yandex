

import vh
from clan_tools.logging.logger import default_log_config
import logging.config
from clan_tools.utils.timing import timing
from clan_tools.vh.workflow import get_package
import click
from clan_tools.data_adapters.YTAdapter import YTAdapter
from clan_tools.vh.operations import run_small_job_op
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

    paths = DictObj(deps_dir=f'{root_dir}/{model_dir}/deps',
                    model_dir=f'{root_dir}/{model_dir}'
                    )
    YTAdapter().create_paths(paths.values())
    spark_criterions_path = f'{paths.model_dir}/spark_criterions'
    spark_path = '//home/comdep-analytics/zedlaa/spark/all_2'
    spark_info_path = f'{paths.model_dir}/spark_info'

    yql_token = 'bakuteev-yql-token'
    

    spark_criterions_target = run_small_job_op(yql_token=yql_token,
                        input=package,
                        _name='spark_criterions',
                        script=dedent(f'scripts/criterions/spark_criterions.py ' +
                                    f'--result_table_path {spark_criterions_path} ' +
                                    f'--spark_path {spark_path} ' 
                                    f'--script spark_criterions.sql'
                                    )).output
    
    with vh.wait_for(spark_criterions_target):
        run_small_job_op(yql_token=yql_token,
                        input=package,
                        _name='spark_info',
                        script=dedent(f'scripts/criterions/spark_criterions.py ' +
                                    f'--result_table_path {spark_info_path} ' +
                                    f'--spark_path {spark_path} ' 
                                    f'--script spark_info.sql'
                                    )).output

    vh.run(wait=(not no_start) and (not update_prod),   
           start=(not no_start) and (not update_prod),
           workflow_guid='4ae35289-f545-4997-971f-0a98a427a644' if update_prod else None,
           label=f'spark_criterions_workflow {"" if update_prod else "test"}',
           project='scoring_of_potential',
           quota='coud-analytics', backend=vh.NirvanaBackend())


if __name__ == '__main__':
    main()
