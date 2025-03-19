import typing

from cloud.ai.speechkit.stt.lib.data.model.dao.records_tags import RecordTagData, RecordTagType
from .model import RecordTagDataRequest


def filter_tags_by_tag_conjunction(
    record_tags: typing.List[str],
    tag_conjunctions: typing.List[typing.Set[RecordTagDataRequest]],
) -> typing.Set[str]:
    """
    YQL query returns all records tags.
    This function will replace record tags by its subset which is requested tag conjunction.
    """
    for tag_conjunction in tag_conjunctions:
        if any(t.value is None or t.negation for t in tag_conjunction):
            raise NotImplementedError('Tag filtering is not supported for type-only or negation filters')

    tags_conjunctions_str: typing.List[typing.Set[str]] = []
    for tag_conjunction in tag_conjunctions:
        tags_conjunctions_str.append({
            RecordTagData(type=t.type, value=t.value).to_str()
            for t in tag_conjunction
        })

    record_tags = set(record_tags)
    suitable_tag_conjunctions = []
    for tag_conjunction in tags_conjunctions_str:
        if tag_conjunction.issubset(record_tags):
            suitable_tag_conjunctions.append(tag_conjunction)

    if len(suitable_tag_conjunctions) > 1:
        # We can allow multiple suitable tag conjunctions, but only in case of composed tags.
        raise ValueError(f'Tags {record_tags} have multiple suitable tag conjunctions: {suitable_tag_conjunctions}')
    elif len(suitable_tag_conjunctions) == 0:
        raise ValueError(f'Tags {record_tags} do not contain any requested tag conjunction')
    else:
        return suitable_tag_conjunctions[0]


tag_type_to_order = {tag: index for index, tag in enumerate(list(RecordTagType))}


def prepare_tags_for_view(tags: typing.Iterable[str], compose: bool) -> typing.List[str]:
    """
    Sort tags by type and optionally compose them to single composite tag.
    """
    tags_data = [RecordTagData.from_str(tag) for tag in tags]
    tags_data = sorted(tags_data, key=lambda tag_data: tag_type_to_order[tag_data.type])

    if compose:
        composite_type = '-'.join(td.type.value for td in tags_data)
        composite_value = '__'.join(td.value for td in tags_data)
        return [f'{composite_type}:{composite_value}']
    else:
        return [td.to_str() for td in tags_data]


def decompose_tag(composed_tag: str) -> typing.List[RecordTagData]:
    tags_types_str, tags_values_str = composed_tag.split(':')
    tags_types = tags_types_str.split('-')
    tags_values = tags_values_str.split('__')
    assert len(tags_types) == len(tags_values)
    return [
        RecordTagData(type=RecordTagType(t), value=v)
        for t, v in zip(tags_types, tags_values)
    ]
