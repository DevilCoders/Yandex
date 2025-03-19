import json
import os
import typing

from cloud.ai.speechkit.stt.lib.data.model.dao import (
    ClusterReferencesInfo, ClusterReferencesArcadia, ClusterReferenceMergeStrategy,
)
from cloud.ai.speechkit.stt.lib.utils.arcadia import arcadia_download

cluster_references_arcadia_dir_path = 'arcadia/cloud/ai/speechkit/stt/lib/text/resources/cluster_references'


def get_cluster_references_from_arcadia(
    topics: typing.List[str],
    lang: str,
    revisions: typing.List[int],
    merge_strategy: ClusterReferenceMergeStrategy,
    arcadia_token: str,
) -> typing.Tuple[
    typing.Dict[str, typing.List[str]],
    ClusterReferencesInfo,
    dict,
]:
    assert len(topics) == len(revisions)

    info = ClusterReferencesInfo(
        descriptors=[
            ClusterReferencesArcadia(topic=topic, lang=lang, revision=revision)
            for topic, revision in zip(topics, revisions)
        ],
        merge_strategy=merge_strategy,
    )
    clusters, extra = get_cluster_references(
        info=info,
        arcadia_token=arcadia_token,
    )
    return clusters, info, extra


def get_cluster_references(
    info: ClusterReferencesInfo,
    arcadia_token: str,
) -> typing.Tuple[typing.Dict[str, typing.List[str]], dict]:
    if info.merge_strategy != ClusterReferenceMergeStrategy.SKIP:
        raise NotImplementedError

    unique_words = set()
    clusters = {}
    extra = {'skipped': []}
    for desc in info.descriptors:
        if not isinstance(desc, ClusterReferencesArcadia):
            raise NotImplementedError

        content = arcadia_download(
            path=os.path.join(cluster_references_arcadia_dir_path, desc.lang, f'{desc.topic}.json'),
            revision=desc.revision,
            token=arcadia_token,
        )
        cr = json.loads(content)
        assert isinstance(cr, dict)

        for center, equal_words in cr.items():
            assert isinstance(center, str)
            assert isinstance(equal_words, list)
            assert all(isinstance(w, str) for w in equal_words)

            if center in unique_words or any(w in unique_words for w in equal_words) > 0:
                extra['skipped'].append({
                    'cluster': {center: equal_words},
                    'descriptor': desc.to_yson(),
                })
            else:
                clusters[center] = equal_words
                unique_words.add(center)
                for w in equal_words:
                    unique_words.add(w)

    return clusters, extra
