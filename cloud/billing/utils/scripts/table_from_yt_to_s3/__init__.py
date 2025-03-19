import json
import time

import boto3
import yt.wrapper as yt


def table_from_yt_to_s3(yt_proxy, yt_token, src_table_path, s3_endpoint_url, access_key_id, secret_access_key,
                        bucket_name):
    yt.config['token'] = yt_token
    yt.config['proxy']['url'] = yt_proxy

    tp = yt.TablePath(src_table_path)
    file_name = str(int(time.time()))

    with open(file_name, "w") as f:
        for row in yt.read_table(tp, format=yt.JsonFormat()):
            f.write(
                json.dumps(row) + '\n'
            )

    boto3.session.Session().client(
        service_name="s3",
        endpoint_url=s3_endpoint_url,
        aws_access_key_id=access_key_id,
        aws_secret_access_key=secret_access_key
    ).upload_file(file_name, bucket_name, file_name)
