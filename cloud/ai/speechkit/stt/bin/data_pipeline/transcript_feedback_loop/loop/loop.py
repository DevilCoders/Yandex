import json

import nirvana.job_context as nv

import cloud.ai.speechkit.stt.lib.data_pipeline.transcript_feedback_loop as transcript


def main():
    op_ctx = nv.context()

    inputs = op_ctx.inputs
    params = op_ctx.parameters

    with open(inputs.get('markup_pool.json')) as f:
        markup_pool_id = json.load(f)['id']
    with open(inputs.get('check_pool.json')) as f:
        check_pool_id = json.load(f)['id']

    if markup_pool_id is None or check_pool_id is None:
        return

    with open(inputs.get('user_urls_map.json')) as f:
        users_urls_map = json.load(f)

    lang = params.get('lang')

    fb_loop = transcript.construct_feedback_loop(inputs, params, users_urls_map, lang)
    fb_loop.loop(markup_pool_id, check_pool_id)
