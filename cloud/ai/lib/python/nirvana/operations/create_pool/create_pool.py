import json

import toloka.client as toloka
import nirvana.job_context as nv

import cloud.ai.lib.python.nirvana.objects as objects


def main():
    op_ctx = nv.context()

    toloka_client = objects.get_toloka_client_by_nirvana_params(op_ctx)

    with open(op_ctx.inputs.get('pool.json')) as f:
        pool_params = toloka.Pool.structure(json.load(f))

    pool = toloka_client.create_pool(pool_params)

    objects.materialize_dict_to_nirvana_output(pool.unstructure(), op_ctx, 'pool.json')
