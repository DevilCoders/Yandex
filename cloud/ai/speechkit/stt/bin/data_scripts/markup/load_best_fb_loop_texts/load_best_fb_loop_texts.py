import json

import nirvana.job_context as nv

import cloud.ai.speechkit.stt.lib.data_pipeline.transcript_feedback_loop as transcript


def main():
    op_ctx = nv.context()

    inputs = op_ctx.inputs
    params = op_ctx.parameters
    outputs = op_ctx.outputs

    with open(inputs.get('markup_pool.json')) as f:
        markup_pool_id = json.load(f)['id']
    with open(inputs.get('check_pool.json')) as f:
        check_pool_id = json.load(f)['id']
    with open(inputs.get('user_urls_map.json')) as f:
        users_urls_map = json.load(f)

    lang = params.get('lang')

    fb_loop = transcript.construct_feedback_loop(inputs, params, users_urls_map, lang)

    url_to_text = {}
    for obj_result in fb_loop.get_results(markup_pool_id, check_pool_id):
        url = obj_result.obj_id
        text = obj_result.solutions[0].marked_object.text
        url_to_text[url] = text

    with open(outputs.get('url_to_text.json'), 'w') as f:
        json.dump(url_to_text, f, indent=4, ensure_ascii=False)
