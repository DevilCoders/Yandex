
import pandas as pd
from datetime import datetime as dt
 
from nile.api.v1 import (
    aggregators as na,
    extractors as ne,
    filters as nf,
    Record
)

from clan_emails.emailing_events_cube.utils.time import get_datetime_from_epoch
from clan_emails.emailing_events_cube.utils.emails import works_with_emails
from clan_emails.emailing_events_cube.CubeJob import CubeJob


def wiki_nurture_streams_to_yt(wiki_adapter, yt_adapter, folder):
    sender_nurture_df = wiki_adapter.get_data(
        table_path='users/oleus/nurture-programs-cloud/', to_pandas=True)
    yt_adapter.save_result('//home/{0}/cubes/emailing_events/wiki_table'.format(folder), 
                                    df=sender_nurture_df,
                                    schema={'Program': 'string', 'Tags': 'string', 'TableName': 'string'},
                                    append=False)
    return sender_nurture_df




class SenderNurturePreprocessor(CubeJob):
    def __init__(self, job, paths_dict, folder, sender_nurture_df) -> None:
        self._job = job
        self._paths_dict = paths_dict
        self._folder = folder
        self._sender_nurture_df = sender_nurture_df

    def job(self):
        job = self._job
        paths_dict = self._paths_dict
        folder = self._folder

        tables = self._sender_nurture_df.TableName.unique()
        tables_jobs = [job.table('//home/cloud_analytics/emailing/sender/{table}'.format(table=table))
                    .project(ne.all(),
                                table_name=ne.const(table))
                    for table in tables]

        def capitalize(s):
            return None if s is None else s.capitalize()

        nurture_tables_path = '//home/{0}/cubes/emailing_events/sender_nurture'.format(
            folder)
        nurture_table = job.concat(
            *tables_jobs
        ) \
            .project(email=ne.custom(works_with_emails, 'email'),
                    group=ne.custom(lambda col1, col2: capitalize(col1 if (col1 is not None) else col2),
                                    'group', 'Group'),
                    experiment_date='experiment_date',
                    table_name='table_name') \
            .put(
            nurture_tables_path,
            schema={'email': str, 'group': str,
                    'experiment_date': str, 'table_name': str}
        )
        return nurture_table


 
    

   

class SenderNurture(CubeJob):
    def __init__(self, job, paths_dict, folder, nurture_job) -> None:
        self._job = job
        self._paths_dict = paths_dict
        self._folder = folder
        self._nurture_job = nurture_job

    def job(self):
        job = self._job
        paths_dict = self._paths_dict
        folder = self._folder

        def sender_date(experiment_date_b):
            experiment_date = experiment_date_b.decode("utf-8") if experiment_date_b is not None else experiment_date_b
            res = str(dt.strptime('2099-12-31', "%Y-%m-%d"))
            try:
                res = str(dt.strptime(experiment_date, "%Y-%m-%d"))
            except:
                pass
           
            return res

        sender_nurture_emails_send = job.table(paths_dict['sender'])\
            .filter(nf.and_(nf.equals('source', b'sender'), nf.equals('event', b'send'))) \
            .project(
                event_time='unixtime',
                email=ne.custom(works_with_emails, 'email'),
                tags='tags'
        )

        sender_nurture = self._nurture_job \
            .join(job.table('//home/{0}/cubes/emailing_events/wiki_table'.format(folder)),
                by_left='table_name', by_right='TableName', type='inner')\
            .join(sender_nurture_emails_send,   by_left=['email', 'Tags'], by_right=['email', 'tags'],  type='left')\
            .project(
                event=ne.const('add_to_nurture_stream'),
                event_time=ne.custom(sender_date,  'experiment_date'),
                email=ne.custom(works_with_emails, 'email'),
                program_name='Program',
                stream_name='group',
                mail_id='Tags',
                mailing_name='Tags',
                mailing_id=ne.const(0)
            )\
            .put(
                '//home/{0}/cubes/emailing_events/sender_events_nurture'.format(
                    folder),
                schema={'mailing_id': int, 'event': str, 'event_time': str, 'email': str,
                        'program_name': str, 'stream_name': str, 'mail_id': str, 'mailing_name': str}
            )
        return sender_nurture