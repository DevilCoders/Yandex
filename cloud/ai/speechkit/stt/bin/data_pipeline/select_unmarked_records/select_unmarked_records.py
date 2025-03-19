#!/usr/bin/python3

import os
import typing
from collections import defaultdict
from multiprocessing.pool import ThreadPool

import nirvana.job_context as nv
import ujson as json

from cloud.ai.lib.python.datetime import now
from cloud.ai.speechkit.stt.lib.data.model.common.id import generate_id
from cloud.ai.speechkit.stt.lib.data.model.dao import (
    Record, RecordTagData, RecordTagType, Mark, MarkupPriorities, MarkupMetadata, TagsMarkupStats,
)
from cloud.ai.speechkit.stt.lib.data.ops.queries import select_records_without_markups_by_tags
from cloud.ai.speechkit.stt.lib.data_pipeline.files import get_name_for_record_audio_file, records_dir
from cloud.ai.speechkit.stt.lib.utils.s3 import create_client


def main():
    started_at = now()

    op_ctx = nv.context()

    outputs = op_ctx.outputs
    params = op_ctx.parameters

    tags: typing.List[str] = params.get('tags')
    tags_limits: typing.List[int] = params.get('tags-limits')
    concurrent_session: typing.Optional[str] = params.get('concurrent-session')

    lang: str = params.get('lang')
    if params.get('mark') == 'ALL':
        mark = None
    else:
        mark = Mark(params.get('mark'))
    min_duration: float = params.get('min-duration')
    max_duration: float = params.get('max-duration')

    records_with_tags = select_records_without_markups_by_tags(
        tags=tags,
        tags_limits=tags_limits,
        mark=mark,
        min_duration_seconds=min_duration,
        max_duration_seconds=max_duration,
        concurrent_session=concurrent_session,
    )

    records = []
    tags_to_duration_seconds: typing.Dict[typing.Tuple[str], float] = defaultdict(float)
    for record, record_tags in records_with_tags:
        records.append(record)
        channels_aware_duration_seconds = record.audio_params.duration_seconds * record.audio_params.channel_count
        tags_to_duration_seconds[record_tags] += channels_aware_duration_seconds

    for tags in tags_to_duration_seconds:
        for tag in tags:
            tag = RecordTagData.from_str(tag)
            if tag.type == RecordTagType.LANG and tag.value != lang:
                raise ValueError(f'some records lang {tag.value} differs from pipeline lang {lang}')

    s3 = create_client()

    os.system(f'mkdir {records_dir}')

    def download_record(record: Record):
        audio = record.download_audio(s3)
        with open(os.path.join(records_dir, get_name_for_record_audio_file(record)), 'wb') as f:
            f.write(audio)

    pool = ThreadPool(processes=16)
    pool.map(download_record, records)

    os.system(f'tar czf {outputs.get("records_files.tar.gz")} {records_dir}')

    with open(outputs.get('records.json'), 'w') as f:
        json.dump([r.to_yson() for r in records], f, indent=4, ensure_ascii=False)

    with open(outputs.get('markup_metadata.json'), 'w') as f:
        json.dump(MarkupMetadata(
            markup_id=generate_id(),
            markup_priority=MarkupPriorities(params.get('priority')),
            tags_stats=[
                TagsMarkupStats(tags=list(tags), duration_seconds=duration_seconds)
                for tags, duration_seconds in tags_to_duration_seconds.items()
            ],
            started_at=started_at,
        ).to_yson(), f, indent=4, ensure_ascii=False)
