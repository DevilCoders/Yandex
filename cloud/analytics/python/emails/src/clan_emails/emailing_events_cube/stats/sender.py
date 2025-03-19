
from nile.api.v1 import (
    aggregators as na,
    extractors as ne,
    filters as nf,
    Record
)

from clan_emails.emailing_events_cube.utils.time import get_datetime_from_epoch
from clan_emails.emailing_events_cube.utils.emails import works_with_emails
from clan_emails.emailing_events_cube.utils.sender import get_sender_event

from clan_emails.emailing_events_cube.CubeJob import CubeJob


def aggregate_tags(groups):
    for key, records in groups:
        result_dict = {}
        longest_tag_len = 0
        for i, rec in enumerate(records):
            if i == 0:
                result_dict['program_name'] = rec['tags']
            if (rec['tags'] is not None) and len(rec['tags']) > longest_tag_len:
                result_dict['mailing_name'] = rec['tags']
        yield Record(key, **result_dict)


class SenderStats(CubeJob):
    def job(self):
        job = self._job
        paths_dict = self._paths_dict
        folder = self._folder

        sender = job.table(paths_dict['sender']) \
            .filter(nf.equals('source', b'sender'))\
            .groupby(
            'event',
            'email',
            'unixtime',
            'message_id'
        )\
            .reduce(aggregate_tags)\
            .project(
            event=ne.custom(get_sender_event, 'event'),
            event_time=ne.custom(lambda x: get_datetime_from_epoch(int(
                x)/1000) if int(x) > 1000000000000 else get_datetime_from_epoch(int(x)), 'unixtime'),
            email=ne.custom(works_with_emails, 'email'),
            mail_id='mailing_name',
            mailing_name='mailing_name',
            program_name='program_name',
            stream_name=ne.const('inapplicable'),
            mailing_id=ne.const(0)
        ) \
            .groupby(
            'event',
            'email',
            'mail_id',
            'mailing_name',
            'program_name',
            'stream_name',
            'mailing_id'
        ) \
            .aggregate(
                event_time=na.min('event_time')
        ) \
            .put(
            '//home/{0}/cubes/emailing_events/sender_events'.format(folder),
            schema={'mailing_id': int, 'event': str, 'event_time': str, 'email': str,
                    'program_name': str, 'stream_name': str, 'mail_id': str, 'mailing_name': str}
        )
        return sender
