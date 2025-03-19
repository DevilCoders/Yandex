import vh
from clan_tools.vh.workflow import get_package
from clan_tools.vh.operations import container, current_date_op
from clan_tools.utils.timing import timing
from clan_tools.logging.logger import default_log_config
import click
import logging.config
logging.config.dictConfig(default_log_config)
logger = logging.getLogger(__name__)


@vh.module(
    out=vh.mkoutput(vh.File),
    local_script=vh.mkoption(bool),
    script=vh.mkoption(str),
    wizard_tables_path=vh.mkoption(str),
    current_date=vh.mkinput(vh.File),
    max_ram=vh.mkoption(int, default=4 * 1024),
    ttl=vh.mkoption(int, default=72 * 3600),
    cpu_guarantee=vh.mkoption(int, default=100)
)
def materialize_wizard_tables(out, local_script, script, wizard_tables_path, current_date, max_ram, ttl, cpu_guarantee):
    package = get_package(package_path='wizard_tables2yt', local_script=local_script,
                          files_to_copy=['src/',  'scripts/',  'setup.py'])
    yt_token_secret = vh.get_yt_token_secret()
    datalens_token = vh.add_global_option('datalens_token', vh.Secret, required=True)
    vh.tgt(
        out, package, current_date,
        recipe='tar xvf {{ package }} '
        + '&& cat {{ current_date }} '
        + '&& PYTHONPATH=./src:$PYTHONPATH '
        + '&& YT_TOKEN={{ yt_token_secret }} DATALENS_TOKEN={{ datalens_token }} '
        + ' python3.7 scripts/{{ script }} '
        + '--wizard_tables_path {{ wizard_tables_path }} '
        + '--yt_token={{ yt_token_secret }} '
        + '&& cat output.json > {{ out }}',
        container=vh.Porto(
            [vh.data_from_str(container, data_type='binary')]),
        hardware_params=vh.HardwareParams(
            max_ram=max_ram, ttl=ttl, cpu_guarantee=cpu_guarantee)
    )


@timing
@click.command()
@click.option('--no_start', is_flag=True)
@click.option('--local_src', is_flag=True)
@click.option('--update_prod', is_flag=True)
def main(local_src=True,  no_start=False, update_prod=False):
    logger.debug('Starting graph')
    logger.info(
        'Updating prod workflow' if update_prod else 'New workflow was created')
    logger.info('Workflow will start' if (not no_start)
                else 'Workflow will not start')

    wizard_tables_path = '//home/cloud_analytics/datalens/wizard_to_yt/wizard_tables'

    current_date = current_date_op().output

    materialize_wizard_tables(
        script='materialize_wizard_tables.py',
        local_script=local_src,
        wizard_tables_path=wizard_tables_path,
        current_date=current_date
    )

    prod_workflow = '7331029d-9370-43f9-86c1-d19948ed91c6'
    test_workflow = None
    vh.run(
        wait=False,  yt_token_secret='robot-clanalytics-yt',
        start=(not no_start) and (not update_prod),
        global_options={
            'datalens_token': 'robot-clanalytics-datalens'},
        workflow_guid=prod_workflow if update_prod else test_workflow,
        quota='coud-analytics',
        label='materialize_wizard_tables',
        backend=vh.NirvanaBackend()
    )


if __name__ == '__main__':
    main()
