import json
import typing
import uuid

import nirvana.job_context as nv

from cloud.ai.speechkit.stt.lib.data.model.dao import MarkupStep, MarkupMetadata
from cloud.ai.speechkit.stt.lib.data_pipeline.toloka import TolokaClient, TolokaEnvironment
from cloud.ai.speechkit.stt.lib.data_pipeline.markup_params import get_pool_params, markup_step_to_overlap_strategy
from cloud.ai.speechkit.stt.lib.data_pipeline.user_bits_urls import add_user_urls_to_tasks


def main():
    op_ctx = nv.context()

    inputs = op_ctx.inputs
    outputs = op_ctx.outputs
    params = op_ctx.parameters

    with open(inputs.get('tasks.json')) as f:
        tasks = json.load(f)
    with open(inputs.get('honeypots.json')) as f:
        honeypots = json.load(f)
    with open(inputs.get('markup_metadata.json')) as f:
        markup_metadata = MarkupMetadata.from_yson(json.load(f))
    with open(inputs.get('user_urls_map.json')) as f:
        users_urls_map = json.load(f)

    markup_step = MarkupStep(params.get('markup-step'))
    lang: str = params.get('lang')
    if params.get('pool-params-override') is not None:
        pool_params_override = json.loads(params.get('pool-params-override'))
    else:
        pool_params_override = {}
    toloka_environment = TolokaEnvironment[params.get('toloka-environment')]
    toloka_token: str = params.get('toloka-token')
    disabled: bool = params.get('disabled')

    if disabled and markup_step == MarkupStep.TRANSCRIPT:
        raise ValueError('Impossible to disable transcript step')

    if len(tasks) < 1 or disabled:
        write_output(None, outputs)
        return

    for task in honeypots:
        task['overlap'] = 10000
    for task in tasks:
        if 'overlap' in task:
            continue
        overlap_strategy = markup_step_to_overlap_strategy.get(markup_step)
        if overlap_strategy is None:
            raise ValueError('Undefined task overlap with unknown overlap strategy')
        task['overlap'] = overlap_strategy.overall

    toloka_client = TolokaClient(oauth_token=toloka_token, lang=lang, environment=toloka_environment)

    all_tasks = tasks + honeypots
    add_user_urls_to_tasks(all_tasks, users_urls_map)

    for task in all_tasks:
        # 'id' input field is automatically generated in DataForge, here we just mock it
        task['input_values']['id'] = f'mock {str(uuid.uuid4())}'

    pool_id = toloka_client.create_pool(
        pool=get_pool_params(
            lang, markup_metadata.markup_id, markup_step, markup_metadata.get_tags(), markup_metadata.markup_priority,
            pool_params_override,
        ),
        tasks=all_tasks,
    )
    write_output(pool_id, outputs)


def write_output(pool_id: typing.Optional[str], outputs):
    with open(outputs.get('pool.json'), 'w') as f:
        json.dump({'id': pool_id}, f)
