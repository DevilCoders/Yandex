import json
from multiprocessing.pool import ThreadPool

import nirvana.job_context as nv

from cloud.ai.speechkit.stt.lib.data.model.common import s3_consts, id
from cloud.ai.speechkit.stt.lib.utils.s3 import create_client


def main():
    op_ctx = nv.context()

    inputs = op_ctx.inputs
    params = op_ctx.parameters
    outputs = op_ctx.outputs

    with open(inputs.get('bits_urls.json')) as f:
        bits_urls = json.load(f)

    s3 = create_client()
    bit_url_prefix = f'https://{s3_consts.cloud_endpoint}/{s3_consts.data_bucket}/'

    upload_to_mds: bool = params.get('upload-to-mds')
    if upload_to_mds:
        mds = create_client(
            endpoint_url=s3_consts.mds_url,
            access_key_id=params.get('mds-aws-access-key-id'),
            secret_access_key=params.get('mds-aws-secret-access-key'),
        )

    def upload_bit(bit_url: str) -> str:
        assert bit_url.startswith(bit_url_prefix)
        bit_s3_key = bit_url[len(bit_url_prefix):]
        user_bit_s3_key = f'Audio/{id.generate_id()}.wav'
        if upload_to_mds:
            audio = s3.get_object(
                Bucket=s3_consts.data_bucket,
                Key=bit_s3_key,
            )['Body'].read()
            mds.put_object(
                Bucket=s3_consts.mds_bucket,
                Key=user_bit_s3_key,
                Body=audio,
            )
            return f'https://{s3_consts.mds_endpoint}/{s3_consts.mds_bucket}/{user_bit_s3_key}'
        else:
            s3.copy_object(
                Bucket=s3_consts.toloka_bucket,
                CopySource=f'{s3_consts.data_bucket}/{bit_s3_key}',
                Key=user_bit_s3_key,
            )
            return f'https://{s3_consts.cloud_endpoint}/{s3_consts.toloka_bucket}/{user_bit_s3_key}'

    pool = ThreadPool(processes=16)
    users_bits_urls = pool.map(upload_bit, bits_urls)

    users_urls_map = {bit_url: user_bit_url for bit_url, user_bit_url in zip(bits_urls, users_bits_urls)}

    with open(outputs.get('user_urls_map.json'), 'w') as f:
        json.dump(users_urls_map, f, indent=4, ensure_ascii=False)
