import json

import nirvana.job_context as nv

import cloud.ai.lib.python.datasource.yt.ops as yt


def main():
    op_ctx = nv.context()

    inputs = op_ctx.inputs
    params = op_ctx.parameters

    table_meta = yt.TableMeta.from_yson(json.loads(params.get('table-meta')))
    table_name: str = params.get('table-name')

    table = yt.Table(meta=table_meta, name=table_name)

    yt.configure_yt_wrapper()
    with open(inputs.get('rows')) as f:
        # TODO: no need to parse json here, support append of file-like objects
        table.append_rows(json.load(f))
