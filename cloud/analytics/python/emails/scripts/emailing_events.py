# -*- coding: utf-8 -*-
import logging.config
import click
import json
from nile.api.v1 import (
    aggregators as na,
    extractors as ne,
    filters as nf,
    Record
)
from nile.api.v1.clusters.base import Cluster
import pandas as pd
import datetime
from clan_tools.data_adapters.nile.NileDataAdapter import NileDataAdapter
from clan_tools.data_adapters.WikiAdapter import WikiAdapter
from clan_tools.data_adapters.YTAdapter import YTAdapter
from clan_tools.logging.logger import default_log_config
from clan_emails.emailing_events_cube.config.paths import PATHS_DICT_PROD, PATHS_DICT_TEST
from clan_emails.emailing_events_cube.stats.sender import SenderStats
from clan_emails.emailing_events_cube.stats.marketo import MarketoOpenEmail, MarketoClickEmail, MarketoEmailDelivered,  MarketoSendEmail
from clan_emails.emailing_events_cube.nurture.marketo import MarketoAddToNurture, MarketoChangeNurtureTrack
from clan_emails.emailing_events_cube.nurture.sender import wiki_nurture_streams_to_yt, SenderNurturePreprocessor, SenderNurture
from clan_emails.emailing_events_cube.Postprocessor import Postprocessor
from clan_emails.emailing_events_cube.stats.notify import NotifyStats
import os
from nile.api.v1 import files as nfl

logging.config.dictConfig(default_log_config)
logger = logging.getLogger(__name__)


def collect_emails_stats(cluster, paths_dict, folder):
    job = cluster.job('Collect mails stat')
    sender = SenderStats(job, paths_dict, folder).job()
    marketo_send_email = MarketoSendEmail(job, paths_dict, folder).job()
    marketo_email_delivered = MarketoEmailDelivered(job, paths_dict, folder).job()
    marketo_open_email = MarketoOpenEmail(job, paths_dict, folder).job()
    marketo_click_email = MarketoClickEmail(job, paths_dict, folder).job()

    job.concat(
        marketo_send_email,
        marketo_email_delivered,
        marketo_open_email,
        marketo_click_email,
        sender
    ) \
        .put(
        '//home/{0}/cubes/emailing_events/email_events'.format(folder),
        schema={'mailing_id': int, 'event': str, 'event_time': str, 'email': str,
                'program_name': str, 'stream_name': str, 'mail_id': str, 'mailing_name': str}
    )
    job.run()

def collect_nurture_emails(cluster, paths_dict, folder, wiki_token, yt_token):
    wiki_adapter = WikiAdapter(token=wiki_token)
    yt_adapter = YTAdapter(token=yt_token)
    sender_nurture_df = wiki_nurture_streams_to_yt(wiki_adapter, yt_adapter, folder)

    job = cluster.job(name='Collect nurture emails')
    change_nurture_track = MarketoAddToNurture(job, paths_dict, folder, wiki_adapter).job()
    add_to_nurture = MarketoChangeNurtureTrack(job, paths_dict, folder, wiki_adapter).job()

    sender_nurture_groups = SenderNurturePreprocessor(job, paths_dict, folder, sender_nurture_df).job()
    sender_nurture = SenderNurture(job, paths_dict, folder, sender_nurture_groups).job()
    job.concat(
        change_nurture_track,
        add_to_nurture,
        sender_nurture
    ) \
        .put(
        '//home/{0}/cubes/emailing_events/add_to_nurture_stream'.format(
            folder),
        schema={'mailing_id': int, 'event': str, 'event_time': str, 'email': str,
                'program_name': str, 'stream_name': str, 'mail_id': str, 'mailing_name': str}
    )
    job.run()


def collect_notify_stats(cluster, paths_dict, folder):
    job = cluster.job('Collect notify stat')
    NotifyStats(job, paths_dict, folder).job()
    job.run()



@click.command()
@click.option('--prod', is_flag=True)
@click.option('--nirvana', is_flag=True)
def main(prod, nirvana):
    wiki_token = os.environ['WIKI_TOKEN']
    yt_token=os.environ['YT_TOKEN']
    yql_token=os.environ['YQL_TOKEN']

    files = [nfl.DevelopPackage('.')] if nirvana else [nfl.DevelopPackage('.'), nfl.DevelopPackage('../lib/clan_tools')]
    nile_cluster = NileDataAdapter(yt_token=yt_token, backend='yt')\
        .env(files=files)

    paths_dict = None
    folder = None
    if not prod:
        paths_dict = PATHS_DICT_TEST
        folder = 'cloud_analytics_test'
        logger.debug('Runining on test')
    elif prod:
        paths_dict = PATHS_DICT_PROD
        folder = 'cloud_analytics'
        logger.debug('Runining on prod')

    collect_emails_stats(nile_cluster, paths_dict, folder)
    collect_nurture_emails(nile_cluster, paths_dict, folder, wiki_token, yt_token)
    collect_notify_stats(nile_cluster, paths_dict, folder)

    table_path = Postprocessor(yql_token, folder).combine_cube()

    with open('output.json', 'w') as f:
        json.dump({"table_path": table_path
        }, f)

    logger.debug("Done for {0}".format(table_path))


if __name__ == '__main__':
    main()
