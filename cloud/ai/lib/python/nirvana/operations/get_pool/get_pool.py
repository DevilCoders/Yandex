import json

import nirvana.job_context as nv

import cloud.ai.lib.python.nirvana.objects as objects


def main():
    op_ctx = nv.context()

    toloka_client = objects.get_toloka_client_by_nirvana_params(op_ctx)

    with open(op_ctx.inputs.get('pool.json')) as f:
        pool_id = json.load(f)['id']

    objects.materialize_dict_to_nirvana_output(
        toloka_client.get_pool(pool_id).unstructure(),
        op_ctx, output_name='pool.json')
