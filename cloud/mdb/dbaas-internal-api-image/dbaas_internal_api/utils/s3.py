# -*- coding: utf-8 -*-
"""
DBaaS Internal API s3 functions
"""

import codecs
from abc import ABC, abstractmethod
from contextlib import closing, contextmanager
from typing import Iterator, TextIO, cast

import boto3
from botocore.client import Config
from botocore.exceptions import ClientError
from flask import current_app

from . import config, logs


class S3Api(ABC):
    """
    Base S3 Client class
    """

    @abstractmethod
    def list_objects(self, prefix: str, delimiter: str = None) -> dict:
        """
        Return object list matching prefix
        """

    @abstractmethod
    @contextmanager
    def get_object_body(self, key: str, encoding='utf-8') -> Iterator[TextIO]:
        """
        Return iterator on object body
        """

    @abstractmethod
    def object_exists(self, key: str) -> bool:
        """
        Return True if object exists
        """


class BotoS3Api(S3Api):
    """
    Simple wrapper around boto3
    """

    def __init__(self, bucket: str) -> None:
        session = boto3.session.Session()
        self._client = session.client(service_name='s3', **config.s3_config(), config=Config(**config.boto_config()))
        self._bucket = bucket

    def list_objects(self, prefix: str, delimiter: str = None) -> dict:
        """
        Return object_list for our bucket
        """
        try:
            kwargs = {}
            if delimiter:
                kwargs['Delimiter'] = delimiter
            return self._client.list_objects(Bucket=self._bucket, Prefix=prefix, **kwargs)
        except self._client.exceptions.NoSuchBucket:
            logs.log_warn('Unable to find bucket in %s in S3', self._bucket)
            return {}

    def object_exists(self, key: str) -> bool:
        """
        Return True if object exists
        """
        try:
            self._client.head_object(Bucket=self._bucket, Key=key)
            return True
        except ClientError:
            return False

    @contextmanager
    def get_object_body(self, key: str, encoding='utf-8') -> Iterator[TextIO]:
        """
        Get object and load its body as json

        Expect that:
        * Object is JSON document
        * Encoded in UTF-8
        """
        s3_object = self._client.get_object(
            Bucket=self._bucket,
            Key=key,
        )
        with closing(s3_object['Body']) as body_fd:
            utf_body_fd = codecs.getreader(encoding)(body_fd)
            yield cast(TextIO, utf_body_fd)


def get_s3_api():
    """
    Factory method for getting S3Api provider
    """
    return current_app.config['S3_PROVIDER']
