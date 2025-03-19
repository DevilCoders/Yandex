from collections import defaultdict
import os
import typing

from cloud.ai.speechkit.stt.lib.data.model.dao import (
    MarkupAssignment,
    MarkupAssignmentStatus,
    RecordBit,
    MarkupData,
    MarkupStep,
)
from cloud.ai.speechkit.stt.lib.data_pipeline.files import get_name_for_record_bit_audio_file_from_record_bit

"""
Combine records bits with its markups from ACCEPTED assignments.
We use only auto-accept or auto-accept + auto-reject for assignments,
so we expect only ACCEPTED, REJECTED and EXPIRED statuses for assignments
after Toloka pool closing. Tasks from REJECTED assignments must be re-made
by Toloka, so we expect that all bits must have at least one markup.
"""


def combine_records_bits_with_markup_data(
    markup_assignments: typing.List[MarkupAssignment],
    records_bits: typing.List[RecordBit],
    records_bits_s3_urls: typing.Dict[str, str],
    markup_step: MarkupStep,
    only_accepted: bool = True,
) -> typing.Dict[str, typing.List[typing.Tuple[MarkupData, str]]]:
    filename_to_record_bit = {
        get_name_for_record_bit_audio_file_from_record_bit(record_bit): record_bit for record_bit in records_bits
    }

    # records_bits_s3_urls contains only bits which marked in this operation,
    # while records_bits contains all bits
    filename_to_record_bit = {
        filename: record_bit
        for filename, record_bit in filename_to_record_bit.items()
        if filename in records_bits_s3_urls.keys()
    }

    assignment_status_to_count = defaultdict(int)

    record_bit_id_to_markup_data_list = defaultdict(list)
    for assignment in markup_assignments:
        status = assignment.data.status
        assignment_status_to_count[status] += 1
        if status not in [
            MarkupAssignmentStatus.ACCEPTED,
            MarkupAssignmentStatus.REJECTED,
            MarkupAssignmentStatus.EXPIRED,
        ]:
            raise ValueError(f'Unexpected status {status} of assignment {assignment.id}')
        if only_accepted and status != MarkupAssignmentStatus.ACCEPTED:
            continue
        for task in assignment.tasks:
            audio_filename = os.path.basename(task.input.audio_s3_obj.key)
            if audio_filename not in filename_to_record_bit:
                # This file is honeypot and it's not contained in records bits
                assert len(task.known_solutions) > 0
                continue
            record_bit = filename_to_record_bit[audio_filename]
            record_bit_id_to_markup_data_list[record_bit.id].append((task, assignment.id))

    if markup_step != MarkupStep.QUALITY_EVALUATION:
        # Every bit must have markup
        assert record_bit_id_to_markup_data_list.keys() == set(b.id for b in filename_to_record_bit.values())

    return record_bit_id_to_markup_data_list
