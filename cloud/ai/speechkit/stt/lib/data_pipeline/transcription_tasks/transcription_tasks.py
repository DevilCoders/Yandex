import typing
from collections import defaultdict

from cloud.ai.speechkit.stt.lib.data_pipeline.files import (
    get_record_id_and_channel_and_start_ms_and_end_ms_by_record_bit_filename,
)


def get_bit_overlap(index: int, size: int, overlap: int, basic_overlap: int, edge_bits_full_overlap: bool) -> int:
    if edge_bits_full_overlap and (index == 0 or index == size - 1):
        return overlap
    return overlap // basic_overlap + (1 if index % basic_overlap < overlap % basic_overlap else 0)


def get_bit_index(start, offset):
    return (start + offset - 1) // offset


def get_bits_overlaps(
    filenames: typing.List[str], overlap: int, offset: int, basic_overlap: int, edge_bits_full_overlap: bool,
) -> typing.Dict[str, int]:
    groups = defaultdict(list)
    for filename in filenames:
        record_id, _, start, _ = get_record_id_and_channel_and_start_ms_and_end_ms_by_record_bit_filename(filename)
        groups[record_id].append((filename, start))

    bits_overlaps = {}
    for record_id, group in groups.items():
        size = len(group)
        for filename, start in group:
            bits_overlaps[filename] = get_bit_overlap(
                get_bit_index(start, offset), size, overlap, basic_overlap, edge_bits_full_overlap,
            )
    return bits_overlaps
