import typing
from dataclasses import dataclass
from itertools import groupby

from cloud.ai.speechkit.stt.lib.data.model.dao import RecordBitMarkup, BitDataTimeInterval, MarkupDataVersions


@dataclass
class RecordBitMarkupData:
    bit_data: typing.Union[BitDataTimeInterval]
    version_to_markups: typing.Dict[MarkupDataVersions, typing.List[RecordBitMarkup]]


@dataclass
class RecordChannelMarkupData:
    channel: int
    bits_markups: typing.List[RecordBitMarkupData]


@dataclass
class RecordMarkupData:
    record_id: str
    channels: typing.List[RecordChannelMarkupData]


def group_records_bits_markups(
    markups_with_bit_data: typing.List[typing.Tuple[RecordBitMarkup, typing.Union[BitDataTimeInterval]]],
) -> typing.List[RecordMarkupData]:
    def record_id_key(
        markup_with_bit_data: typing.Tuple[RecordBitMarkup, typing.Union[BitDataTimeInterval]],
    ) -> str:
        return markup_with_bit_data[0].record_id

    def record_bit_channel_key(
        markup_with_bit_data: typing.Tuple[RecordBitMarkup, typing.Union[BitDataTimeInterval]],
    ) -> int:
        return markup_with_bit_data[1].channel

    def record_bit_id_key(
        markup_with_bit_data: typing.Tuple[RecordBitMarkup, typing.Union[BitDataTimeInterval]],
    ) -> str:
        # Record bit id is like "record_id/split_timestamp/channel/bit_index", so sorting
        # by it will arrange markups by start_ms
        return markup_with_bit_data[0].bit_id

    def record_bit_markup_version_key(
        markup_with_bit_data: typing.Tuple[RecordBitMarkup, typing.Union[BitDataTimeInterval]],
    ) -> str:
        return markup_with_bit_data[0].markup_data.version.value

    def record_bit_markup_id_key(
        markup_with_bit_data: typing.Tuple[RecordBitMarkup, typing.Union[BitDataTimeInterval]],
    ) -> str:
        return markup_with_bit_data[0].id

    result = []

    markups_with_bit_data = sorted(
        markups_with_bit_data,
        key=record_id_key,
    )
    for record_id, record_markups_with_bit_data in groupby(
        markups_with_bit_data,
        key=record_id_key,
    ):
        record_markup_data = RecordMarkupData(record_id=record_id, channels=[])

        record_markups_with_bit_data = sorted(
            record_markups_with_bit_data,
            key=record_bit_channel_key,
        )
        for channel, record_channel_markups_with_bit_data in groupby(
            record_markups_with_bit_data,
            key=record_bit_channel_key,
        ):
            record_channel_markup_data = RecordChannelMarkupData(channel=channel, bits_markups=[])

            record_channel_markups_with_bit_data = sorted(
                record_channel_markups_with_bit_data,
                key=record_bit_id_key,
            )
            for bit_id, record_bit_markups_with_bit_data in groupby(
                record_channel_markups_with_bit_data,
                key=record_bit_id_key,
            ):
                record_bit_markups_with_bit_data = sorted(
                    list(record_bit_markups_with_bit_data),
                    key=record_bit_markup_version_key,
                )
                bit_data = record_bit_markups_with_bit_data[0][1]
                version_to_markups = {}
                record_bit_markup_data = RecordBitMarkupData(bit_data=bit_data, version_to_markups=version_to_markups)
                for markup_version_str, record_bit_markups_with_bit_data_by_version in groupby(
                    record_bit_markups_with_bit_data,
                    key=record_bit_markup_version_key,
                ):
                    # This sort is not necessary, but let's do it for determinism
                    record_bit_markups_with_bit_data_by_version = sorted(
                        list(record_bit_markups_with_bit_data_by_version),
                        key=record_bit_markup_id_key,
                    )

                    version_to_markups[MarkupDataVersions(markup_version_str)] = [
                        x[0] for x in record_bit_markups_with_bit_data_by_version
                    ]

                record_channel_markup_data.bits_markups.append(record_bit_markup_data)

            record_markup_data.channels.append(record_channel_markup_data)

        result.append(record_markup_data)

    return result
