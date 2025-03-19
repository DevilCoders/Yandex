import argparse
import json

from cloud.ai.lib.python.datasource.yt.ops import Table
from cloud.ai.lib.python.datetime import now
from cloud.ai.speechkit.stt.lib.data.model.dao import RecordTag, RecordTagData, RecordTagType
from cloud.ai.speechkit.stt.lib.data.ops.yt import table_records_tags_meta


def _parse_args():
    parser = argparse.ArgumentParser(formatter_class=argparse.ArgumentDefaultsHelpFormatter)
    parser.add_argument(
        '-records_ids_path',
        help='Path to JSON file with records ids',
    )
    parser.add_argument(
        '-tag_value',
        help='OTHER tag value',
    )
    return parser.parse_args()


def main():
    run(**vars(_parse_args()))


def run(records_ids_path: str, tag_value: str):
    with open(records_ids_path) as f:
        records_ids = set(json.load(f))

    received_at = now()
    records_tags = []
    tag_data = RecordTagData(type=RecordTagType.OTHER, value=tag_value)
    for record_id in records_ids:
        records_tags.append(
            RecordTag.add_by_record_id(
                record_id=record_id,
                data=tag_data,
                received_at=received_at,
            )
        )

    table_name = Table.get_name(received_at)
    table_records_tags = Table(meta=table_records_tags_meta, name=table_name)
    table_records_tags.append_objects(records_tags)
