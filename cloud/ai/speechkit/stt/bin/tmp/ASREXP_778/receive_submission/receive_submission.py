import json
import os

import nirvana.job_context as nv

from cloud.ai.lib.python.datetime import now
from cloud.ai.speechkit.stt.lib.data.model.common.id import generate_id
from cloud.ai.lib.python.datasource.yt.ops import Table
from cloud.ai.speechkit.stt.lib.tmp.ASREXP_778 import markup_records_indexes, table_submissions_meta, Submission
from cloud.ai.speechkit.stt.lib.utils.s3 import create_client


def main():
    op_ctx = nv.context()

    outputs = op_ctx.outputs
    params = op_ctx.parameters

    login: str = params.get('login')
    records_s3_bucket: str = params.get('records-s3-bucket')
    records_s3_dir_key: str = params.get('records-s3-dir-key')

    s3 = create_client()

    if records_s3_dir_key.endswith('/'):
        records_s3_dir_key = records_s3_dir_key[:-1]

    os.system(f'mkdir mid')

    # В S3 папке должны лежать файлы 0.mid ... 10199.mid
    # При этом случайные 100 из них мы выбрали для разметки
    for i in markup_records_indexes:
        audio = s3.get_object(
            Bucket=records_s3_bucket,
            Key=f'{records_s3_dir_key}/{i}.mid',
        )['Body'].read()
        with open(f'mid/{i}.mid', 'wb') as f:
            f.write(audio)

    submission = Submission(
        login=login,
        id=generate_id(),
        records_s3_bucket=records_s3_bucket,
        records_s3_dir_key=records_s3_dir_key,
        received_at=now(),
    )

    with open(outputs.get('submission.json'), 'w') as f:
        json.dump(submission.to_yson(), f, indent=4)

    os.system(f'tar czf {outputs.get("mid.tar.gz")} mid')

    table_submissions = Table(meta=table_submissions_meta, name='table')
    table_submissions.append_objects([submission])
