import argparse

import yt.wrapper as yt

from cloud.ai.lib.python.datasource.yt.ops import Table
from cloud.ai.speechkit.stt.lib.data.model.dao import (
    Record,
    RecordAudio,
    RecordTag,
    RecordTagType,
    RecordSourceImport,
    ImportSource,
    RecordJoin,
)
from cloud.ai.speechkit.stt.lib.data.ops.yt import (
    table_records_meta,
    table_records_audio_meta,
    table_records_tags_meta,
    table_records_joins_meta,
)

voice_recorder_tables = [
    "2020-11-20",
    "2020-11-10",
    "2020-11-11",
    "2020-11-13",
    "2020-11-21",
    "2020-11-09",
    "2020-09-25",
    "2020-11-24",
    "2020-11-26",
    "2020-11-27",
    "2020-11-28",
    "2020-11-29",
    "2020-12-29",
    "2020-12-28",
    "2021-02-05",
    "2021-02-12",
    "2021-02-20",
    "2021-02-19",
    "2021-02-25",
    "2021-03-02",
    "2021-03-03",
    "2021-03-04",
    "2021-03-05",
    "2021-02-27",
    "2021-02-28",
    "2021-04-03",
    "2021-04-04"
]


def _parse_args():
    parser = argparse.ArgumentParser()
    parser.add_argument(
        '-dry_run',
        action='store_true',
        help='Dry run: do not change YT tables, only log changes.',
    )
    return parser.parse_args()


def main():
    yt.config["read_parallel"]["enable"] = True
    yt.config["read_parallel"]["max_thread_count"] = 8
    yt.config["write_parallel"]["enable"] = True
    yt.config["write_parallel"]["max_thread_count"] = 8

    run(**vars(_parse_args()))


def run(dry_run: bool):
    if dry_run:
        print('DRY RUN')

    for table_name in voice_recorder_tables:
        print(f'Processing table {table_name}', flush=True)
        table_records = Table(meta=table_records_meta, name=table_name)
        table_records_audio = Table(meta=table_records_audio_meta, name=table_name)
        table_records_tags = Table(meta=table_records_tags_meta, name=table_name)
        table_records_joins = Table(meta=table_records_joins_meta, name=table_name)

        print("Start reading records...")
        raw_records = []
        failed_records_id = set()
        for record_row in yt.read_table(table_records.path):
            raw_records.append(record_row)

        records = list(map(lambda arg: Record.from_yson(arg), raw_records))

        failed_records_id = list(filter(lambda record: isinstance(record.source, RecordSourceImport) and record.source.source == ImportSource.VOICE_RECORDER_V1 and record.source.data.asr_mark == "FAILED_AUDIO", records))
        failed_records_id = set(map(lambda record: record.id, failed_records_id))

        print(f'Failed audios count: {len(failed_records_id)}')

        records = list(filter(lambda record: record.id not in failed_records_id, records))

        records_audio = list(filter(lambda record_audio: record_audio[b'record_id'].decode("ascii") not in failed_records_id, yt.read_table(table_records_audio.path, format=yt.YsonFormat(encoding=None))))

        records_tags = list(map(lambda arg: RecordTag.from_yson(arg), yt.read_table(table_records_tags.path)))
        records_tags = list(filter(lambda records_tag: records_tag.record_id not in failed_records_id, records_tags))

        records_joins = list(map(lambda arg: RecordJoin.from_yson(arg), yt.read_table(table_records_joins.path)))
        records_joins = list(filter(lambda record_join: record_join.record_id not in failed_records_id, records_joins))

        if dry_run:
            print("Finished dry run")
            continue

        yt.remove(table_records.path)
        table_records.append_objects(records)

        yt.remove(table_records_audio.path)
        table_records_audio.append_rows(sorted(records_audio, key=lambda ra: ra[b'record_id'].decode("ascii")))

        yt.remove(table_records_tags.path)
        table_records_tags.append_objects(records_tags)

        yt.remove(table_records_joins.path)
        table_records_joins.append_objects(records_joins)
