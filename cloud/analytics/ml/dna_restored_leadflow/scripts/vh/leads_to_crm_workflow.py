from requests import post
import vh
from clan_tools.logging.logger import default_log_config
import logging.config
from clan_tools.utils.timing import timing
from clan_tools.vh.operations import run_yql_script
import click

logging.config.dictConfig(default_log_config)
logger = logging.getLogger(__name__)


@timing
@click.command()
@click.option('--local_src', is_flag=True, default=False)
@click.option('--is_prod', is_flag=True, default=False)
@click.option('--with_start', is_flag=True, default=False)
def main(local_src: bool = False,  is_prod: bool = False, with_start: bool = False) -> None:
    logger.debug('Starting graph')
    logger.info('Updating production workflow' if is_prod else 'Testing workflow')
    logger.info('Workflow will start' if with_start else 'Workflow will not start')

    postfix = "" if is_prod else "_test"
    output_path = "//home/cloud_analytics/export/crm/update_call_center_leads/update_leads" + postfix

    run_yql_script(
        path='src/dna_restored_leadflow/yql/leads_to_crm.sql',
        yql_token='robot-clan-pii-yt-yql_token',
        query_options={'output': output_path},
        _name='To CRM')

    vh.run(
        wait=False,
        keep_going=True,
        start=with_start,
        workflow_guid=('168157a8-0143-4bea-838c-21e98f9dcb68' if is_prod else None),
        quota='coud-analytics',
        backend=vh.NirvanaBackend()
    )


if __name__ == '__main__':
    main()
