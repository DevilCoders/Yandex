
from nile.api.v1 import (
    aggregators as na,
    extractors as ne,
    filters as nf,
    Record
)
from functools import partial
from clan_emails.emailing_events_cube.utils.time import get_datetime_from_epoch
from clan_emails.emailing_events_cube.utils.emails import works_with_emails
from clan_emails.emailing_events_cube.utils.sender import get_sender_event

from clan_emails.emailing_events_cube.CubeJob import CubeJob
from clan_emails.emailing_events_cube.utils.marketo import get_mail_id


class MarketoNurture(CubeJob):
    def __init__(self, job, paths_dict, folder, wiki_adapter) -> None:
        self._job = job
        self._paths_dict = paths_dict
        self._folder = folder
        self._wiki_adapter = wiki_adapter
        self._tracks_ids = self.get_track_ids()

    def get_track_ids(self):
        columns, rows = self._wiki_adapter.get_data(
            table_path='users/ktereshin/ProgramId-StreamId-Dictionary/')
        track_ids = {}
        for list_ in rows:
            if list_[3] != '':
                if int(list_[2]) not in track_ids:
                    track_ids[int(list_[2])] = {}
                else:
                    pass
                track_ids[int(list_[2])][int(list_[3])] = {
                    'program_name': list_[0], 'stream_name': list_[1]}
        return track_ids

    @staticmethod
    def get_program_name(track_ids, program_id, stream_id):
        if program_id in track_ids:
            if stream_id in track_ids[program_id]:
                return track_ids[program_id][stream_id]['program_name']
        return 'unknown'

    @staticmethod
    def get_stream_name(track_ids, program_id, stream_id):
        if program_id in track_ids:
            if stream_id in track_ids[program_id]:
                return track_ids[program_id][stream_id]['stream_name']
        return 'unknown'

    @property
    def pget_program_name(self):
        return partial(self.get_program_name, self._tracks_ids)

    @property
    def pget_stream_name(self):
        return partial(self.get_stream_name, self._tracks_ids)

    def job(self):
        pass


class MarketoChangeNurtureTrack(MarketoNurture):

    def job(self):
        job = self._job

        change_nurture_track = job.table('//home/cloud_analytics/import/marketo/change_nurture_track') \
            .unique(
            'marketo_id'
        ) \
            .project(
            event=ne.const('add_to_nurture_stream'),
            event_time=ne.custom(
                lambda x: get_datetime_from_epoch(x), 'created'),
            email=ne.custom(works_with_emails, 'email'),
            program_name=ne.custom(
                self.pget_program_name, 'program_id', 'new_track_id'),
            stream_name=ne.custom(self.pget_stream_name,
                                  'program_id', 'new_track_id'),
            mail_id=ne.const('inapplicable'),
            mailing_name=ne.const('inapplicable'),
            mailing_id=ne.const(0)

        )
        return change_nurture_track


class MarketoAddToNurture(MarketoNurture):
    def job(self):
        job = self._job
        add_to_nurture = job.table('//home/cloud_analytics/import/marketo/add_to_nurture') \
            .unique(
            'marketo_id'
        ) \
            .project(
            event=ne.const('add_to_nurture_stream'),
            event_time=ne.custom(
                lambda x: get_datetime_from_epoch(x), 'created'),
            email=ne.custom(works_with_emails, 'email'),
            program_name=ne.custom(
                self.pget_program_name, 'program_id', 'track_id'),
            stream_name=ne.custom(self.pget_stream_name,
                                  'program_id', 'track_id'),
            mail_id=ne.const('inapplicable'),
            mailing_name=ne.const('inapplicable'),
            mailing_id=ne.const(0)

        )
        return add_to_nurture
