
import vh
from clan_tools.vh.workflow import get_package
from clan_tools.utils.timing import timing
import click
import logging.config
from clan_tools.logging.logger import default_log_config

logging.config.dictConfig(default_log_config)
logger = logging.getLogger(__name__)


@vh.module(
    out=vh.mkoutput(vh.File),
    local_script=vh.mkoption(bool),
    script=vh.mkoption(str),
    wizard_tables_path=vh.mkoption(str),
    max_ram=vh.mkoption(int, default=4 * 1024),
    ttl=vh.mkoption(int, default=72 * 3600),
    cpu_guarantee=vh.mkoption(int, default=100)
)
def add_datalens_table(out, local_script, script, wizard_tables_path, max_ram, ttl, cpu_guarantee):
    package = get_package(package_path='wizard_tables2yt', local_script=local_script, files_to_copy=['src/', 'scripts/', 'setup.py'])
    datalens_url = vh.add_global_option('datalens_url', str, required=True)
    yt_path = vh.add_global_option('yt_path', str, required=True)
    is_regular = vh.add_global_option('is_regular', bool, required=True)
    yt_token_secret = vh.get_yt_token_secret()
    datalens_token = vh.add_global_option('datalens_token', vh.Secret, required=True)

    recipe = '''
        tar xvf {{ package }} \\
        && PYTHONPATH=./src:$PYTHONPATH \\
        && YT_TOKEN={{ yt_token_secret }} DATALENS_TOKEN={{ datalens_token }} \\
        python3.7 scripts/{{ script }} \\
            --wizard_tables_path {{ wizard_tables_path }} \\
            --datalens_url={{ datalens_url }} \\
            --yt_path={{ yt_path }} \\
            --is_regular={{ is_regular }} \\
            --yt_token={{ yt_token_secret }} \\
        && cat output.json > {{ out }}
    '''
    vh.tgt(
        out, package, recipe=recipe,
        container=vh.Porto([vh.data_from_str('0c21d2b3-b9d9-4f31-841a-042515588c94', data_type='binary')]),
        hardware_params=vh.HardwareParams(max_ram=max_ram, ttl=ttl, cpu_guarantee=cpu_guarantee)
    )


@timing
@click.command()
@click.option('--local_src', is_flag=True, default=False)
@click.option('--is_prod', is_flag=True, default=False)
@click.option('--with_start', is_flag=True, default=False)
def main(local_src=False,  is_prod=False, with_start=False):
    logger.debug('Starting graph')
    logger.info('Updating prod workflow' if is_prod else 'Testing workflow')
    logger.info('Workflow will start' if with_start else 'Workflow will not start')

    wizard_tables_path = '//home/cloud_analytics/ml/mql_marketing/result/crm/wizard_tables'
    add_datalens_table(script='add_datalens_table.py',
                       local_script=local_src,
                       wizard_tables_path=wizard_tables_path)

    prod_workflow = '63aa30a9-511e-4979-b021-0dd0ed91c0df'
    test_workflow = '1d85fda7-aae7-4dd1-81f3-440ef9a0a4a3'
    vh.run(wait=False,  yt_token_secret='robot-clanalytics-yt',
           start=with_start,
           global_options={
               'datalens_url': 'https://datalens.yandex-team.ru/wizard/0rzx0ga5bmkit',
               'yt_path': '//home/cloud_analytics/ml/mql_marketing/result/crm/mal_leads_update',
               'is_regular': False,
               'datalens_token': 'robot-clanalytics-datalens'
           },
           workflow_guid=prod_workflow if is_prod else test_workflow,
           quota='coud-analytics',
           label='add_datalens_table',
           backend=vh.NirvanaBackend())


if __name__ == '__main__':
    main()
