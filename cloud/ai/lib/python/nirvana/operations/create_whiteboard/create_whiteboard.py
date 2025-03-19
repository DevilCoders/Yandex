import json

import nirvana.job_context as nv

import cloud.ai.lib.python.nirvana.objects as objects


def main():
    op_ctx = nv.context()

    with open(op_ctx.inputs.get('wb.json')) as f:
        id = json.load(f)['id']

    wb_meta = objects.create_whiteboard_meta_from_nirvana_workflow(
        id=id,
        cls=op_ctx.parameters.get('whiteboard-class'),
        nv_context=op_ctx,
        s3_dir_path=objects.S3Object(
            endpoint=op_ctx.parameters.get('s3-endpoint'),
            bucket=op_ctx.parameters.get('s3-bucket'),
            key=op_ctx.parameters.get('s3-dir-key'),
        )
    )

    objects.materialize_object_to_nirvana_output(wb_meta, op_ctx, 'meta.json')
