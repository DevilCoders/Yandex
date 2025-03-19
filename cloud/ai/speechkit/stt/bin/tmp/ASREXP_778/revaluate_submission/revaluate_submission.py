import json
import os

import yt.wrapper as yt
import nirvana.job_context as nv

from cloud.ai.lib.python.datasource.yt.ops import Table
from cloud.ai.speechkit.stt.lib.tmp.ASREXP_778 import (
    revaluate_markup_records_indexes_small, table_submissions_meta, Submission,
)
from cloud.ai.speechkit.stt.lib.utils.s3 import create_client


def main():
    op_ctx = nv.context()

    outputs = op_ctx.outputs
    params = op_ctx.parameters

    submission_id: str = params.get('submission-id')

    submission = None
    table_submissions = Table(meta=table_submissions_meta, name='table')
    for row in yt.read_table(table_submissions.path):
        if row['id'] == submission_id:
            submission = Submission.from_yson(row)
            break

    assert submission is not None, f'submission {submission_id} not found'

    s3 = create_client()

    os.system(f'mkdir mid')

    # В S3 папке должны лежать файлы 0.mid ... 10199.mid
    # При этом случайные 300 из них мы выбрали для финальной разметки
    for i in revaluate_markup_records_indexes_small:
        audio = s3.get_object(
            Bucket=submission.records_s3_bucket,
            Key=f'{submission.records_s3_dir_key}/{i}.mid',
        )['Body'].read()
        with open(f'mid/{i}.mid', 'wb') as f:
            f.write(audio)

    with open(outputs.get('submission.json'), 'w') as f:
        json.dump(submission.to_yson(), f, indent=4)

    os.system(f'tar czf {outputs.get("mid.tar.gz")} mid')
