import json

import nirvana.job_context as nv

from cloud.ai.lib.python import datetime
from cloud.ai.speechkit.stt.lib.data.model.dao import MarkupParams, MarkupMetadata
from cloud.ai.speechkit.stt.lib.data.ops.yt import table_markups_params_meta
from cloud.ai.lib.python.datasource.yt.model import generate_json_options_for_table, get_table_name


def main():
    op_ctx = nv.context()

    inputs = op_ctx.inputs
    outputs = op_ctx.outputs

    params = {}
    for param_key in inputs.get_named_items('params.json').keys():
        with open(inputs.get('params.json', link_name=param_key)) as f:
            specific_params = json.load(f)
            if len(specific_params) > 0:
                params[param_key] = specific_params
    with open(inputs.get('markup_metadata.json')) as f:
        markup_metadata = MarkupMetadata.from_yson(json.load(f))

    received_at = datetime.now()
    table_name = get_table_name(received_at)

    with open(outputs.get('markup_params.json'), 'w') as f:
        json.dump(MarkupParams(
            markup_id=markup_metadata.markup_id,
            params=params,
            received_at=received_at,
        ).to_yson(), f, indent=4, ensure_ascii=False)

    with open(outputs.get('markup_params_table.json'), 'w') as f:
        json.dump(generate_json_options_for_table(meta=table_markups_params_meta, name=table_name), f,
                  indent=4, ensure_ascii=False)
