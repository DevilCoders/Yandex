import argparse
import json

from cloud.ai.lib.python.datetime import now
from cloud.ai.lib.python.datasource.yt.ops import Table
from cloud.ai.speechkit.stt.lib.data.model.dao import (
    RecordTagData,
    RecordJoin,
    RecognitionPlainTranscript,
    JoinDataExemplar,
)
from cloud.ai.speechkit.stt.lib.data.ops.queries import select_records_ids_by_tag
from cloud.ai.speechkit.stt.lib.data.ops.yt import table_records_joins_meta


def _parse_args():
    parser = argparse.ArgumentParser(
        description='Import exemplar joins for existing records',
        formatter_class=argparse.ArgumentDefaultsHelpFormatter,
    )
    parser.add_argument(
        '-references_path', help='Path to JSON file with references, with mapping {record_id: reference_text}'
    )
    parser.add_argument(
        '-tag',
        help='Records tag (to validate records ids from references file)',
        type=str,
    )
    return parser.parse_args()


def main():
    run(**vars(_parse_args()))


def run(references_path, tag):
    with open(references_path) as f:
        record_id_to_reference = {record_id: reference for record_id, reference in json.loads(f.read()).items()}
    tag_data = RecordTagData.from_str(tag)
    records_ids_with_tag = select_records_ids_by_tag(tag_data)
    assert record_id_to_reference.keys() == set(records_ids_with_tag)

    received_at = now()
    records_joins = []
    for record_id, reference in record_id_to_reference.items():
        records_joins.append(
            RecordJoin.create_by_record_id(
                record_id=record_id,
                recognition=RecognitionPlainTranscript(text=reference),
                join_data=JoinDataExemplar(),
                received_at=received_at,
                other=None,
            )
        )

    table_records_joins = Table(meta=table_records_joins_meta, name=Table.get_name(received_at))
    table_records_joins.append_objects(records_joins)
