from typing import Iterable, List, Dict
from itertools import groupby

from cloud.ai.speechkit.stt.lib.data.model.dao import RecordTagData, Mark

from ..queries.records_by_tags_marks import select_records_by_tags_marks, SelectRecordsByTagsMarksRow


def select_records(tags_str: Iterable[str], marks_str: Iterable[str]) -> List[Dict]:
    def get_mark(mark: str) -> Mark:
        mark = mark.lower()
        if mark == 'train':
            return Mark.TRAIN
        if mark == 'test':
            return Mark.TEST
        if mark == 'val':
            return Mark.VAL
        raise ValueError('Unknown mark type')

    def record_id_key(row: SelectRecordsByTagsMarksRow) -> str:
        return row.record_id

    def record_tag_key(row: SelectRecordsByTagsMarksRow) -> str:
        return row.tag_data.to_str()

    rows = select_records_by_tags_marks(
        (RecordTagData.from_str(tag) for tag in tags_str), (get_mark(mark) for mark in marks_str)
    )
    rows = sorted(rows, key=record_id_key)

    result = []

    for record_id, record_rows in groupby(rows, key=record_id_key):
        record_rows = list(record_rows)
        record_rows = sorted(record_rows, key=record_tag_key)

        s3_obj = record_rows[0].s3_obj
        spec = {
            'audio_encoding': record_rows[0].req_params.get_audio_encoding().value,
            'audio_channel_count': 1,
            'sample_rate_hertz': record_rows[0].req_params.get_sample_rate_hertz(),
        }

        tags = [tag for tag, _ in groupby(record_rows, key=record_tag_key)]

        result.append(
            {
                'id': record_id,
                's3_obj': s3_obj.to_yson(),
                'tags': tags,
                'spec': spec,
                'ref': None,
            }
        )
    return result
