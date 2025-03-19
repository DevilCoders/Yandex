"""
s3 client to dataproc job logs
"""

import logging

from flask import current_app
from typing import Tuple
from contextlib import closing

import boto3
from botocore.client import Config

from dbaas_common import tracing
from .api import DataprocJobLog, DataprocJobLogClientError, filter_necessary_parts
from ...utils.request_context import get_x_request_id
from ...utils.config import dataproc_joblog_config

MiB = 2**20


class DataprocJobLogAccessDenied(DataprocJobLogClientError):
    """
    Operations on user's bucket denied
    """


class DataprocJobLogNoSuchBucket(DataprocJobLogClientError):
    """
    Not found user's bucket in Object Storage
    """


class DataprocJobLogNoSuchKey(DataprocJobLogClientError):
    """
    Not found object
    """


class DataprocJobLogNotFound(DataprocJobLogClientError):
    """
    Not found logs for job
    """


class DataprocS3JobLogClient(DataprocJobLog):
    """
    Provider for dataproc job on object storage
    """

    def __init__(self, bucket: str, iam_token: str, endpoint_url: str = None, region_name: str = None) -> None:
        config = dataproc_joblog_config()
        self._bucket = bucket
        boto_config = Config(
            connect_timeout=1.0,
            read_timeout=5.0,
            user_agent="dbaas-internal-api",
            retries={
                'max_attempts': 2,
            },
        )
        self._client = boto3.client(
            's3',
            region_name=region_name if region_name else config['region'],
            endpoint_url='https://' + (endpoint_url if endpoint_url else config['endpoint']),
            aws_access_key_id='fake-access-id',
            aws_secret_access_key='fake-secret-key',
            config=boto_config,
        )

        def inject_iam_token(request, **kwargs):
            request.headers.add_header('X-YaCloud-SubjectToken', iam_token)

        self._client.meta.events.register_first('before-sign.s3', inject_iam_token)

        self.logger = logging.LoggerAdapter(
            logging.getLogger(current_app.config['LOGCONFIG_BACKGROUND_LOGGER']),
            extra={
                'request_id': get_x_request_id(),
            },
        )

    def get_content(  # type: ignore
        self, cluster_id: str, job_id: str, offset: int = 0, limit: int = 1 * MiB
    ) -> Tuple[bytes, int]:
        """
        Gather data proc job log chuck
        """
        content, page_token = bytes([]), 0
        if limit == 0:
            return content, page_token
        all_parts = self._list_job_parts(cluster_id, job_id)
        if len(all_parts) == 0:
            raise DataprocJobLogNotFound(f'Not found any log files for job')
        all_parts.sort(key=lambda part: part['Key'])
        parts, new_page_token = filter_necessary_parts(all_parts, offset, limit)

        for part in parts:
            self.logger.debug(f'Requesting dataproc job log part {part["Key"]}')
            try:
                content += self._get_part(part)
            except self._client.exceptions.NoSuchKey as e:
                message = f'Failed to retrieve Data Proc job log part {part["Key"]}: {e.response["Message"]}'
                self.logger.warn(message, e)
                raise DataprocJobLogNoSuchKey(message)
        return content, new_page_token

    @tracing.trace('S3 GetDataprocJobLogPart')
    def _get_part(self, part: dict) -> bytes:
        """
        Download single part of driveroutput and truncate it from both sides
        """
        s3_object = self._client.get_object(Bucket=self._bucket, Key=part['Key'])
        content = None
        seek = part.get('Seek', 0)
        amount = part.get('Amount', part['Size'])
        with closing(s3_object['Body']) as fh:
            content = fh.read(amount + seek)
        if seek != 0 and seek < len(content):
            content = content[seek:]
        return content

    @tracing.trace('S3 ListDataprocJobLogParts')
    def _list_job_parts(self, cluster_id: str, job_id: str):
        """
        List all driveroutput objects from object storage for requested (cluster_id, job_id)
        """
        objects = []
        try:
            page_no = 1
            for page in self._client.get_paginator("list_objects_v2").paginate(
                Bucket=self._bucket, Prefix=f'dataproc/clusters/{cluster_id}/jobs/{job_id}/driveroutput.'
            ):
                self.logger.debug(
                    f'Retrieved list job log parts, cluster_id {cluster_id}, job_id {job_id}, page {page_no}'
                )
                if page.get('Contents') is None or page.get('KeyCount') == 0:
                    break
                objects.extend([{'Key': obj['Key'], 'Size': obj['Size']} for obj in page['Contents']])
                page_no = page_no + 1
            return objects
        except self._client.exceptions.ClientError as e:
            if e.response['Error']['Message'] == 'Access Denied':
                raise DataprocJobLogAccessDenied(f'Access denied for listing objects in bucket {self._bucket}')
            raise
        except self._client.exceptions.NoSuchBucket:
            raise DataprocJobLogNoSuchBucket(f'Not such bucket {self._bucket}')
        return objects
