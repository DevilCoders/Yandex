import os

import boto3
from botocore.errorfactory import ClientError


def upload_file(file, bucket_name):
    s3_session = boto3.session.Session()
    s3_client = s3_session.client(
        service_name='s3',
        endpoint_url='https://storage.yandexcloud.net'
    )

    file_basename = os.path.basename(file)
    if not check_uploaded(s3_client, bucket_name, file_basename):
        print("sending file '%s' to the bucket '%s'" % (file, bucket_name))
        s3_client.upload_file(file, bucket_name, file_basename)
    else:
        print("file '%s' is already in the bucket '%s'" % (file, bucket_name))

    return "https://storage.yandexcloud.net/" + bucket_name + "/" + file_basename


# Taken from https://stackoverflow.com/a/44979500/2304212
def check_uploaded(s3_client, bucket_name, cloud_file_name):
    try:
        s3_client.head_object(Bucket=bucket_name, Key=cloud_file_name)
    except ClientError as err:
        if int(err.response["Error"]["Code"]) == 404:
            # The object does not exist
            return False
        else:
            raise err
    else:
        # The object does exist
        return True
