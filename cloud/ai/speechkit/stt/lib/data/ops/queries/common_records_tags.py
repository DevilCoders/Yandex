import typing

from cloud.ai.speechkit.stt.lib.data.model.dao import RecordTagData


def get_tags_filter(tags: typing.Iterable[RecordTagData], data_column='records_tags.`data`'):
    return 'OR'.join(
        f"""
        (
            Yson::LookupString({data_column}, "type") = '{tag.type.value}' AND
            Yson::LookupString({data_column}, "value") = '{tag.value}'
        )
        """
        for tag in tags
    )
