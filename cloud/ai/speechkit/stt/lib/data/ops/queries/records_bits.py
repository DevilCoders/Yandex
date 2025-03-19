from dataclasses import dataclass
import os
import typing

from cloud.ai.speechkit.stt.lib.data.model.dao import (
    S3Object,
    RecordBit,
    AudioObfuscationData,
    get_audio_obfuscation_data,
)
from .select import run_yql_select_query
from cloud.ai.speechkit.stt.lib.data.model.tags import (
    parse_tag_conjunctions,
    validate_tag_conjunctions,
)
from .records_tags import tag_conjunctions_to_yql_query
from ..yt import table_records_bits_meta, table_records_bits_markups_meta


@dataclass
class RecordBitWithObfuscationData:
    record_bit: RecordBit
    obfuscated_audio_s3_obj: S3Object
    audio_obfuscation_data: AudioObfuscationData


def select_records_split_by_tags(
    tags: typing.List[str],
    split_at_str: str,
) -> typing.List[RecordBitWithObfuscationData]:
    tag_conjunctions = parse_tag_conjunctions(tags)
    validate_tag_conjunctions(tag_conjunctions)

    yql_query = f"""
{tag_conjunctions_to_yql_query(tag_conjunctions, os.environ['YT_PROXY'], bits=True)}

SELECT
    records_bits.`record_id` AS record_id,
    records_bits.`split_id` AS split_id,
    records_bits.`id` AS id,
    records_bits.`split_data` AS split_data,
    records_bits.`bit_data` AS bit_data,
    records_bits.`convert_data` AS convert_data,
    records_bits.`mark` AS mark,
    records_bits.`s3_obj` AS s3_obj,
    records_bits.`audio_params` AS audio_params,
    records_bits.`received_at` AS received_at,
    records_bits.`other` AS other,
    Yson::YPath(records_bits_markups.`markup_data`, "/input/audio_s3_obj") AS obfuscated_audio_s3_obj,
    records_bits_markups.`audio_obfuscation_data` AS audio_obfuscation_data,
FROM
    LIKE(`{table_records_bits_meta.dir_path}`, "%") AS records_bits
INNER JOIN
    $records_with_specified_tags AS records_with_specified_tags
ON
    records_bits.`id` = records_with_specified_tags.`record_id`
INNER JOIN
    LIKE(`{table_records_bits_markups_meta.dir_path}`, "%") AS records_bits_markups
ON
    records_bits.`id` = records_bits_markups.`bit_id`
WHERE
    records_bits.`received_at` = "{split_at_str}";
"""

    tables, _ = run_yql_select_query(yql_query)
    rows = tables[0]
    result = []
    records_bits_ids = set()
    for row in rows:
        record_bit = RecordBit.from_yson(row)
        if record_bit.id in records_bits_ids:
            continue  # multiple markup rows for each bit, but obfuscated audio s3 obj and data will be the same
        records_bits_ids.add(record_bit.id)
        obfuscated_audio_s3_obj = S3Object.from_yson(row['obfuscated_audio_s3_obj'])
        audio_obfuscation_data = get_audio_obfuscation_data(row['audio_obfuscation_data'])
        result.append(
            RecordBitWithObfuscationData(
                record_bit=record_bit,
                obfuscated_audio_s3_obj=obfuscated_audio_s3_obj,
                audio_obfuscation_data=audio_obfuscation_data,
            )
        )
    return result
