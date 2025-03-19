
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
from clan_emails.emailing_events_cube.utils.marketo import get_mail_id


class MarketoSendEmail(CubeJob):
    def job(self):
        job = self._job
        paths_dict = self._paths_dict
        folder = self._folder

        send_email = job.table(paths_dict['send_email']) \
            .project(
            event=ne.const('email_sended'),
            event_time=ne.custom(
                lambda x: get_datetime_from_epoch(x), 'created'),
            email=ne.custom(works_with_emails, 'email'),
            mail_id=ne.custom(get_mail_id, 'mailing_name'),
            mailing_name='mailing_name',
            program_name=ne.const('inapplicable'),
            stream_name=ne.const('inapplicable'),
            mailing_id='mailing_id'
        ) \
            .put(
            '//home/{0}/cubes/emailing_events/send_email'.format(folder),
            schema={'mailing_id': int, 'event': str, 'event_time': str, 'email': str,
                    'program_name': str, 'stream_name': str, 'mail_id': str, 'mailing_name': str}
        )
        return send_email


class MarketoEmailDelivered(CubeJob):
    def job(self):
        job = self._job
        paths_dict = self._paths_dict
        folder = self._folder
        email_delivered = job.table(paths_dict['email_delivered']) \
            .project(
            event=ne.const('email_deliveried'),
            event_time=ne.custom(
                lambda x: get_datetime_from_epoch(x), 'created'),
            email=ne.custom(works_with_emails, 'email'),
            mail_id=ne.custom(get_mail_id, 'mailing_name'),
            mailing_name='mailing_name',
            program_name=ne.const('inapplicable'),
            stream_name=ne.const('inapplicable'),
            mailing_id='mailing_id'

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
            '//home/{0}/cubes/emailing_events/email_delivered'.format(folder),
            schema={'mailing_id': int, 'event': str, 'event_time': str, 'email': str,
                    'program_name': str, 'stream_name': str, 'mail_id': str, 'mailing_name': str}
        )
        return email_delivered


class MarketoOpenEmail(CubeJob):
    def job(self):
        job = self._job
        paths_dict = self._paths_dict
        folder = self._folder
        open_email = job.table(paths_dict['open_email']) \
            .project(
            event=ne.const('email_opened'),
            event_time=ne.custom(
                lambda x: get_datetime_from_epoch(x), 'created'),
            email=ne.custom(works_with_emails, 'email'),
            mail_id=ne.custom(get_mail_id, 'mailing_name'),
            mailing_name='mailing_name',
            program_name=ne.const('inapplicable'),
            stream_name=ne.const('inapplicable'),
            mailing_id='mailing_id'

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
            '//home/{0}/cubes/emailing_events/open_email'.format(folder),
            schema={'mailing_id': int, 'event': str, 'event_time': str, 'email': str,
                    'program_name': str, 'stream_name': str, 'mail_id': str, 'mailing_name': str}
        )
        return open_email


class MarketoClickEmail(CubeJob):
    def job(self):
        job = self._job
        paths_dict = self._paths_dict
        folder = self._folder
        click_email = job.table(paths_dict['click_email']) \
            .unique(
            'marketo_id'
        ) \
            .project(
            event=ne.const('email_clicked'),
            event_time=ne.custom(
                lambda x: get_datetime_from_epoch(x), 'created'),
            email=ne.custom(works_with_emails, 'email'),
            mail_id=ne.custom(get_mail_id, 'mailing_name'),
            mailing_name='mailing_name',
            program_name=ne.const('inapplicable'),
            stream_name=ne.const('inapplicable'),
            mailing_id='mailing_id'
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
            '//home/{0}/cubes/emailing_events/click_email'.format(folder),
            schema={'mailing_id': int, 'event': str, 'event_time': str, 'email': str,
                    'program_name': str, 'stream_name': str, 'mail_id': str, 'mailing_name': str}
        )

        return click_email
