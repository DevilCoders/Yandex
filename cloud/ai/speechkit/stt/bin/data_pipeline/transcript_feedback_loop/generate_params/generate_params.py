import json

import nirvana.job_context as nv

import cloud.ai.speechkit.stt.lib.data_pipeline.markup_params as markup_params


def main():
    op_ctx = nv.context()

    params = op_ctx.parameters
    outputs = op_ctx.outputs

    with open(outputs.get('params.json'), 'w') as f:
        json.dump(markup_params.get_feedback_loop_params(params.get('lang')).to_yson(), f, indent=4, ensure_ascii=False)
