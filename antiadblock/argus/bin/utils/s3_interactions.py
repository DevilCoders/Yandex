# -*- coding: utf-8 -*-
import mimetypes
import os

import boto3
from botocore.errorfactory import ClientError
from retry import retry


class S3Uploader:
    S3_ENDPOINT_URL = 'https://s3.mds.yandex.net'
    BUCKET = 'argus'
    S3_PATH = 'http://argus.s3.mds.yandex.net/'

    def __init__(self, logger) -> None:
        self.client = boto3.client(
            's3',
            endpoint_url=self.S3_ENDPOINT_URL,
            aws_access_key_id=os.environ['S3_KEY_ID'],
            aws_secret_access_key=os.environ['S3_SECRET_KEY'],
        )
        self.logger = logger

    def get_object(self, filename: str, default=None):
        try:
            obj = self.client.get_object(Bucket=S3Uploader.BUCKET, Key=filename)
            return obj['Body'].read()
        except Exception:
            return default

    def upload_file(self, file_path: str, rewrite: bool = False) -> str:
        # сначала проверим что такой файл еще не заливали
        filename = os.path.basename(file_path)
        try:
            self.client.head_object(Bucket=S3Uploader.BUCKET, Key=filename)
            if rewrite:
                self._upload_file(file_path, filename)
            self.logger.info("File already exist on s3 - {0}".format(filename))
        except ClientError:
            # файла нет - заливаем его
            self._upload_file(file_path, filename)
        return S3Uploader.S3_PATH + filename

    @retry(tries=3, delay=1, backoff=3)
    def _upload_file(self, file_path: str, filename: str) -> None:
        with open(file_path, 'rb') as data:
            self.client.upload_fileobj(
                data,
                S3Uploader.BUCKET,
                filename,
                ExtraArgs={'ContentType': mimetypes.guess_type(filename)[0] or 'text/plain'},
            )
