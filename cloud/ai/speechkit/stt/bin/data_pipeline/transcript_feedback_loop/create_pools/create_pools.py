import json
import typing

import nirvana.job_context as nv

import cloud.ai.speechkit.stt.lib.data_pipeline.transcript_feedback_loop as transcript
import cloud.ai.speechkit.stt.lib.data.model.dao as dao
import cloud.ai.lib.python.log as log

logger = log.get_logger(__name__)


def main():
    op_ctx = nv.context()

    inputs = op_ctx.inputs
    params = op_ctx.parameters
    outputs = op_ctx.outputs

    if params.get('disabled'):
        write_output(None, None, outputs)
        return

    with open(inputs.get('user_urls_map.json')) as f:
        users_urls_map = json.load(f)

    lang = params.get('lang')

    fb_loop = transcript.construct_feedback_loop(inputs, params, users_urls_map, lang)

    if len(list(fb_loop.pool_input_objects)) == 0:
        logger.info('no objects to markup')
        write_output(None, None, outputs)
        return

    with open(inputs.get('control_tasks.json')) as f:
        control_objects = [transcript.task_to_control_objects(t, users_urls_map) for t in json.load(f)]

    with open(inputs.get('markup_metadata.json')) as f:
        markup_metadata = dao.MarkupMetadata.from_yson(json.load(f))
    markup_pool, check_pool = fb_loop.create_pools(
        control_objects,
        transcript.get_pool_config(
            lang, markup_metadata.markup_id, dao.MarkupStep.TRANSCRIPT, markup_metadata.get_tags(),
            markup_metadata.markup_priority,
        ),
        transcript.get_pool_config(
            lang, markup_metadata.markup_id, dao.MarkupStep.CHECK_TRANSCRIPT, markup_metadata.get_tags(),
            markup_metadata.markup_priority,
        ),
        check_projects=False,
    )

    write_output(markup_pool.id, check_pool.id, outputs)


def write_output(markup_pool_id: typing.Optional[str], check_pool_id: typing.Optional[str], outputs):
    with open(outputs.get('markup_pool.json'), 'w') as f:
        json.dump({'id': markup_pool_id}, f, indent=4, ensure_ascii=False)
    with open(outputs.get('check_pool.json'), 'w') as f:
        json.dump({'id': check_pool_id}, f, indent=4, ensure_ascii=False)
