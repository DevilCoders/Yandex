import nirvana.job_context as nv

import cloud.ai.lib.python.datasource.s3 as s3


def main():
    with open(nv.context().inputs.get('data'), 'rb') as f:
        params = nv.context().parameters
        s3.create_client(
            endpoint=params.get('endpoint'),
            access_key_id=params.get('access-key-id'),
            secret_access_key=params.get('secret-access-key'),
        ).put_object(Bucket=params.get('bucket'), Key=params.get('key'), Body=f)
