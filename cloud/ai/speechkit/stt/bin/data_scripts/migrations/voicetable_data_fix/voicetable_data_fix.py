import argparse

import yt.wrapper as yt
import yt.yson as yson

from cloud.ai.lib.python.datasource.yt.ops import Table
from cloud.ai.speechkit.stt.lib.data.model.dao import (
    Record,
    RecordTag,
    RecordTagType,
    RecordSourceImport,
    ImportSource,
    RecordJoin,
)
from cloud.ai.speechkit.stt.lib.data.ops.yt import (
    table_records_meta,
    table_records_tags_meta,
    table_records_joins_meta,
)

vt_tables = [
    "2019-01-01",
    "2019-01-11",
    "2019-01-21",
    "2019-01-31",
    "2019-02-10",
    "2019-02-20",
    "2019-03-02",
    "2019-03-12",
    "2019-03-22",
    "2019-04-01",
    "2019-04-11",
    "2019-04-21",
    "2019-05-01",
    "2019-05-11",
    "2019-05-21",
    "2019-05-31",
    "2019-06-10",
    "2019-06-20",
    "2019-06-30",
    "2019-07-10",
    "2019-07-20",
    "2019-07-30",
    "2019-08-09",
    "2019-08-19",
    "2019-08-29",
    "2019-09-08",
    "2019-09-18",
    "2019-09-28",
    "2019-10-08",
    "2019-10-18",
    "2019-10-28",
    "2019-11-07",
    "2019-11-17",
    "2019-11-27",
    "2019-12-07",
    "2019-12-17",
    "2019-12-27",
    "2020-01-06",
    "2020-01-16",
    "2020-01-26",
    "2020-02-05",
    "2020-02-15",
    "2020-02-25",
    "2020-03-06",
    "2020-03-16",
    "2020-03-26",
    "2020-04-05",
    "2020-04-15",
    "2020-04-25",
    "2020-05-05",
]

dit_tables = ["2020-05-26"]


class ProcessResult:
    def __init__(self):
        self.duplicated_vt = 0
        self.removed_dit = 0
        self.fixed_tags = 0
        self.total_records = 0
        self.records_remaining = 0
        self.total_records_tags = 0
        self.records_tags_remaining = 0
        self.total_records_joins = 0
        self.records_joins_remaining = 0

    def print_summary(self):
        print(
            f"""
Process table result:
    Duplicates from VT: {self.duplicated_vt}
    DIT removed: {self.removed_dit}
    Tags fixed: {self.fixed_tags}
    Records -- total: {self.total_records}, remaining: {self.records_remaining}, delta: {self.total_records - self.records_remaining}
    Records tags -- total: {self.total_records_tags}, remaining: {self.records_tags_remaining}, delta: {self.total_records_tags - self.records_tags_remaining}
    Records joins -- total: {self.total_records_joins}, remaining: {self.records_joins_remaining}, delta: {self.total_records_joins - self.records_joins_remaining}
"""
        )

    @staticmethod
    def merge(a, b):
        res = ProcessResult()
        res.duplicated_vt = a.duplicated_vt + b.duplicated_vt
        res.removed_dit = a.removed_dit + b.removed_dit
        res.fixed_tags = a.fixed_tags + b.fixed_tags
        res.total_records = a.total_records + b.total_records
        res.records_remaining = a.records_remaining + b.records_remaining
        res.total_records_tags = a.total_records_tags + b.total_records_tags
        res.records_tags_remaining = a.records_tags_remaining + b.records_tags_remaining
        res.total_records_joins = a.total_records_joins + b.total_records_joins
        res.records_joins_remaining = a.records_joins_remaining + b.records_joins_remaining

        return res

    def inc_duplicated_yt(self):
        self.duplicated_vt += 1

    def inc_removed_dit(self):
        self.removed_dit += 1

    def inc_fixed_tags(self):
        self.fixed_tags += 1

    def inc_total_records(self):
        self.total_records += 1

    def inc_records_remaining(self):
        self.records_remaining += 1

    def inc_total_records_tags(self):
        self.total_records_tags += 1

    def inc_records_tags_remaining(self):
        self.records_tags_remaining += 1

    def inc_total_records_joins(self):
        self.total_records_joins += 1

    def inc_records_joins_remaining(self):
        self.records_joins_remaining += 1


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


def load_duplicates():
    result = set()

    for row in yt.read_table("//home/mlcloud/hotckiss/duplicated"):
        result.add(row['uttid'])

    return result


def run(dry_run: bool):
    from_table = '0000-01-01'
    to_table = '9999-12-31'
    if dry_run:
        print('DRY RUN')

    print("Loading duplicates...")
    duplicated_uttids = load_duplicates()

    uttids_ = set()  # unique uttids

    for table_name in yt.list(table_records_meta.dir_path):
        # only VT table or dit table
        if (table_name in vt_tables or table_name in dit_tables) and from_table <= table_name <= to_table:

            print(f'Processing table {table_name}')
            summary = ProcessResult()
            table_records = Table(meta=table_records_meta, name=table_name)
            table_records_tags = Table(meta=table_records_tags_meta, name=table_name)
            table_records_joins = Table(meta=table_records_joins_meta, name=table_name)

            print("Start reading records...")
            records = []  # all records without duplicates

            records_ids = set()  # unique records -- voicetable duplicates not allowed
            records_ids_voicetable = set()  # NOT unique records from voicetable
            dup = 0  # duplicates count

            for record_row in yt.read_table(table_records.path, format=yt.YsonFormat(encoding=None)):
                record = Record.from_yson(record_row)
                summary.inc_total_records()
                # process record if imported from voicetable
                if isinstance(record.source, RecordSourceImport) and record.source.source == ImportSource.VOICETABLE:
                    uttid = record.source.data.uttid.decode("utf-8")
                    records_ids_voicetable.add(record.id)
                    # skip if record is duplicated and previously added
                    if uttid in duplicated_uttids and uttid in uttids_:
                        dup += 1
                        summary.inc_duplicated_yt()

                    # add record if not duplicated (voicetable only)
                    else:
                        uttids_.add(uttid)
                        records_ids.add(record.id)
                        # acoustic is fixed in Record.from_yson
                        record.audio_params.acoustic = RecordTag.sanitize_value(record.audio_params.acoustic)
                        records.append(record)
                else:
                    records_ids.add(record.id)
                    records.append(record)

            print("Duplicates: " + str(dup))

            print("Start reading tags...")
            dit_ids = set()  # record_id of dit records
            records_tags = []
            for record_tag_row in yt.read_table(table_records_tags.path, format=yt.YsonFormat(encoding=None)):
                record_tag = RecordTag.from_yson(record_tag_row)
                summary.inc_total_records_tags()
                # do not add deprecated dit tags id, add to blacklist
                if record_tag.data.type == RecordTagType.IMPORT and record_tag.data.value == "yt-table_dit-686":
                    summary.inc_removed_dit()
                    dit_ids.add(record_tag.record_id)

                # add record tag only for non-duplicates (dit already skipped)
                elif record_tag.record_id in records_ids:
                    records_tags.append(record_tag)

            # filter dit records, now records correct
            records = list(filter(lambda rec: rec.id not in dit_ids, records))

            # fix tags values
            for i in range(len(records_tags)):
                old_ = records_tags[i]

                # fix only VT tags
                if old_.record_id in records_ids_voicetable:
                    new_value = RecordTag.sanitize_value(old_.data.value)
                    if old_.data.value != new_value:
                        summary.inc_fixed_tags()
                    records_tags[i].data.value = new_value

            print("Start reading joins...")
            records_joins = []
            for record_join_row in yt.read_table(table_records_joins.path, format=yt.YsonFormat(encoding=None)):
                record_join = RecordJoin.from_yson(record_join_row)
                summary.inc_total_records_joins()
                join_record_id = record_join.record_id.decode("utf-8")
                # remove dit joins
                if join_record_id in dit_ids:
                    continue

                # if record from VT and was removed as a duplicate
                if join_record_id in records_ids_voicetable and join_record_id not in records_ids:
                    continue

                records_joins.append(record_join)

            summary.records_remaining = len(records)
            summary.records_tags_remaining = len(records_tags)
            summary.records_joins_remaining = len(records_joins)

            if dry_run:
                print("Finished dry run")
                summary.print_summary()
                continue

            records_table_path = f'{table_records_meta.dir_path}/{table_name}'

            yt.remove(records_table_path)
            yt.create('table', records_table_path, attributes=yson.json_to_yson(table_records_meta.attrs))
            yt.write_table(records_table_path, (r.to_yson() for r in sorted(records)))

            records_tags_table_path = f'{table_records_tags_meta.dir_path}/{table_name}'

            yt.remove(records_tags_table_path)
            yt.create('table', records_tags_table_path, attributes=yson.json_to_yson(table_records_tags_meta.attrs))
            yt.write_table(records_tags_table_path, (t.to_yson() for t in sorted(records_tags)))

            records_joins_table_path = f'{table_records_joins_meta.dir_path}/{table_name}'

            yt.remove(records_joins_table_path)
            yt.create('table', records_joins_table_path, attributes=yson.json_to_yson(table_records_joins_meta.attrs))
            yt.write_table(records_joins_table_path, (t.to_yson() for t in sorted(records_joins)))

            print("Finished run")

            summary.print_summary()
