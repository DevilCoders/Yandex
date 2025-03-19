import json

import nirvana.job_context as nv
import toloka.client as toloka

import cloud.ai.lib.python.nirvana.objects as objects


def main():
    op_ctx = nv.context()

    toloka_client = objects.get_toloka_client_by_nirvana_params(op_ctx)
    statuses: list[str] = op_ctx.parameters.get('statuses')

    with open(op_ctx.inputs.get('pool.json')) as f:
        pool_id = json.load(f)['id']

    objects.materialize_dicts_to_nirvana_output([
        assignment.unstructure() for assignment in
        toloka_client.get_assignments(pool_id=pool_id, status=[
            toloka.Assignment.Status[status] for status in statuses
        ])
    ], op_ctx, output_name='assignments.json')
