import json
import typing

import nirvana.job_context as nv

from cloud.ai.lib.python.datasource.yt.ops import Table
from cloud.ai.speechkit.stt.lib.text.slice.application import PreparedSlice, infer_slices
from cloud.ai.speechkit.stt.lib.text.slice.generation import table_slices_meta, SliceDescriptor


def main():
    op_ctx = nv.context()
    params = op_ctx.parameters
    outputs = op_ctx.outputs

    prepared_slices = get_prepared_slices(
        custom=params.get('custom'),
        names=params.get('names') or [],
        predicates=params.get('predicates') or [],
        arcadia_token=params.get('arcadia-token'),
    )

    with open(outputs.get('slices.json'), 'w') as f:
        json.dump([s.to_yson() for s in prepared_slices], f, indent=4, ensure_ascii=False)


def get_prepared_slices(
    custom: bool,
    names: typing.List[str],
    predicates: typing.List[str],
    arcadia_token: str,
) -> typing.List[PreparedSlice]:
    if len(names) == 0:
        return []

    # Custom predicates are specified directly in options.
    # Usual predicates are selected by name from YT table.

    assert len(names) == len(set(names))
    if custom:
        assert len(names) == len(predicates)
        prepared_slices = [
            PreparedSlice(name=name, predicate=predicate)
            for name, predicate in zip(names, predicates)
        ]
        infer_slices('тест', prepared_slices)  # for fast failure
        return prepared_slices

    assert len(predicates) == 0
    table = Table(meta=table_slices_meta, name='table')
    stored_slices = [SliceDescriptor.from_yson(row) for row in table.read_rows()]
    slices = [s for s in stored_slices if s.name in set(names)]
    assert len(names) == len(slices), 'all requested slices must be in YT table'
    return [s.prepare(arcadia_token) for s in slices]
