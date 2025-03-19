import json

import nirvana.job_context as nv

from cloud.ai.speechkit.stt.lib.data.model.dao import ClusterReferenceMergeStrategy
from cloud.ai.speechkit.stt.lib.text.cluster_references import get_cluster_references_from_arcadia


def main():
    op_ctx = nv.context()
    params = op_ctx.parameters
    outputs = op_ctx.outputs

    clusters_references, info, extra = get_cluster_references_from_arcadia(
        topics=params.get('topics'),
        lang=params.get('lang'),
        revisions=params.get('revisions'),
        merge_strategy=ClusterReferenceMergeStrategy(params.get('merge-strategy')),
        arcadia_token=params.get('arcadia-token'),
    )

    with open(outputs.get('cluster_references.json'), 'w') as f:
        json.dump(clusters_references, f, indent=4, ensure_ascii=False)

    with open(outputs.get('info.json'), 'w') as f:
        json.dump(info.to_yson(), f, indent=4, ensure_ascii=False)

    with open(outputs.get('extra.json'), 'w') as f:
        json.dump(extra, f, indent=4, ensure_ascii=False)
