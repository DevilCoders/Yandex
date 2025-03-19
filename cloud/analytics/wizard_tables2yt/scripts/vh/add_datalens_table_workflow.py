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
    package = get_package(package_path='wizard_tables2yt', local_script=local_script,
                          files_to_copy=['src/', 'scripts/', 'setup.py'])
    datalens_url = vh.add_global_option('datalens_url', str, required=True)
    yt_path = vh.add_global_option('yt_path', str, required=True)
    is_regular = vh.add_global_option('is_regular', bool, required=True)
    yt_token_secret = vh.get_yt_token_secret()
    datalens_token = vh.add_global_option(
        'datalens_token', vh.Secret, required=True)

    vh.tgt(out, package,
           recipe='tar xvf {{ package }} '
           + '&& PYTHONPATH=./src:$PYTHONPATH '
           + '&& YT_TOKEN={{ yt_token_secret }} DATALENS_TOKEN={{ datalens_token }} '
           + ' python3.7 scripts/{{ script }} '
           + '--wizard_tables_path {{ wizard_tables_path }} '
           + '--datalens_url={{ datalens_url }} '
           + '--yt_path={{ yt_path }} '
           + '--is_regular={{ is_regular }} '
           + '--yt_token={{ yt_token_secret }} '
           + '&& cat output.json > {{ out }}',
           container=vh.Porto(
               [vh.data_from_str('0c21d2b3-b9d9-4f31-841a-042515588c94', data_type='binary')]),
           hardware_params=vh.HardwareParams(
               max_ram=max_ram, ttl=ttl, cpu_guarantee=cpu_guarantee)
           )


@timing
@click.command()
@click.option('--prod', is_flag=True)
@click.option('--no_start', is_flag=True)
@click.option('--local_src', is_flag=True)
@click.option('--update_prod', is_flag=True)
def main(local_src=True,  prod=False, no_start=False, update_prod=False):
    logger.debug('Starting graph')
    logger.info(
        'Updating prod workflow' if update_prod else 'New workflow was created')
    logger.info('Workflow will start' if (not no_start)
                else 'Workflow will not start')

    wizard_tables_path = '//home/cloud_analytics/datalens/wizard_to_yt/wizard_tables'
    add_datalens_table(script='add_datalens_table.py',
                       local_script=local_src,
                       wizard_tables_path=wizard_tables_path)

    prod_workflow = '942077f1-8553-45d7-8d7d-1b224fc9db4a'
    test_workflow = None
    vh.run(wait=False,  yt_token_secret='robot-clanalytics-yt',
           start=(not no_start) and (not update_prod),
           global_options={
               'datalens_url': 'https://datalens.yandex-team.ru/wizard/unpwkl4tl7iyp-test',
               'yt_path': '//home/cloud_analytics/tmp/wizard_test2',
               'is_regular': False,
               'datalens_token': 'robot-clanalytics-datalens'
           },
           workflow_guid=prod_workflow if update_prod else test_workflow,
           quota='coud-analytics',
           label='add_datalens_table',
           backend=vh.NirvanaBackend())


if __name__ == '__main__':
    main()
