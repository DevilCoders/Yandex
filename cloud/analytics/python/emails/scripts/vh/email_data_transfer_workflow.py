from textwrap import dedent
import vh
from clan_tools.vh.workflow import get_package
from clan_tools.vh.operations import run_small_job_op, run_job_op, copy_to_ch
from clan_tools.utils.timing import timing
from clan_tools.utils.conf import read_conf
import click
import logging.config
config = read_conf('config/logger.yml')
logging.config.dictConfig(config)
logger = logging.getLogger(__name__)


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
    package = get_package(
        package_path='python/emails', local_script=local_src,
        files_to_copy=[
            'config/', 'src/', 'scripts/', 'setup.py']
    )

    transfer_result = run_small_job_op(
        input=package, _name='transfer_mail_data',
        script=f'scripts/transfer_mail_data.py {"--prod" if prod else ""}',
        yql_token='robot-clanalytics-yql'
    ).output

    extract_table_path = vh.op(id='3ee7ffc0-c86c-4afa-8663-ec1d2eb410f3')

    table_path = extract_table_path(inp=transfer_result, field='table_path')

    with vh.wait_for(table_path):
        emailing_events = run_small_job_op(
            input=package, _name='emailing_events',
            script=f'scripts/emailing_events.py  --nirvana {"--prod" if prod else ""}',
            yt_token='robot-clanalytics-yt',
            yql_token='robot-clanalytics-yql',
            wiki_token='robot-clanalytics-wiki',
        ).output

    copy_to_ch(
        yt_table=table_path.out,
        ch_table=f"{'test_' if not prod else ''}emails_delivery_clicks",
        ch_primary_key='unixtime, message_id, source',
        ch_partition_key='event',
        ch_table_schema=None,
        yt_table_is_file=True
    )

    emailing_events_table_path = extract_table_path(
        inp=emailing_events, field='table_path')

    copy_to_ch(
        yt_table=emailing_events_table_path.out,
        ch_table=f"{'test_' if not prod else ''}emailing_events",
        ch_primary_key='event_time, email, mail_id, event, stream_name',
        ch_table_schema=(
            'billing_account_id Nullable(String), ' +
            'event String, ' +
            'event_time DateTime, ' +
            'mail_id String, ' +
            'mailing_name String, ' +
            'program_name String, ' +
            'stream_name String, ' +
            'mailing_id Int64, ' +
            'email String, ' +
            'puid Nullable(String), ' +
            'ba_state Nullable(String), ' +
            'block_reason Nullable(String), ' +
            'is_fraud Nullable(Int64), ' +
            'ba_usage_status Nullable(String), ' +
            'segment Nullable(String), ' +
            'account_name Nullable(String), ' +
            'cloud_created_time Nullable(String), ' +
            'ba_created_time Nullable(String), ' +
            'first_trial_consumption_time Nullable(String), ' +
            'first_paid_consumption_time Nullable(String)'
        ),
        # ch_partition_key='event, program_name',
        yt_table_is_file=True
    )

    with vh.wait_for(emailing_events_table_path):
        dash_dataset = run_small_job_op(input=package, _name='nurturing_dashboard_dataset',
                                        script=f'scripts/nurturing_dashboard_dataset.py {"--prod" if prod else ""}',
                                        yql_token='robot-clanalytics-yql').output

        with vh.wait_for(dash_dataset):
            run_job_op(input=package, _name='stat_tests',
                       script='scripts/stat_tests.py '
                       + '--dataset_path //home/cloud_analytics/import/emails/nurturing_dashboard_dataset '
                       + '--result_path //home/cloud_analytics/import/emails/stat_tests',
                       yt_token='robot-clanalytics-yt')

        dash_dataset_no_overlap = run_small_job_op(
            input=package, _name='nurturing_dashboard_dataset',
            script=f'scripts/nurturing_dashboard_dataset.py --no_overlap {"--prod" if prod else ""}',
            yql_token='robot-clanalytics-yql'
        ).output

        with vh.wait_for(dash_dataset_no_overlap):
            run_job_op(input=package, _name='stat_tests_no_overlap',
                       script='scripts/stat_tests.py '
                       + '--dataset_path //home/cloud_analytics/import/emails/nurturing_dashboard_dataset_no_overlap '
                       + '--result_path //home/cloud_analytics/import/emails/stat_tests_no_overlap',
                       yt_token='robot-clanalytics-yt')

    vh.run(
        wait=False,
        start=(not no_start) and (not update_prod),
        workflow_guid='f66a5a72-59a5-44e8-ab32-978a03dacc28' if update_prod else None,
        quota='coud-analytics',
        label='emailing',
        backend=vh.NirvanaBackend()
    )


if __name__ == '__main__':
    main()
