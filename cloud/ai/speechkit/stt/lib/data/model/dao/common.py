from dataclasses import dataclass
from enum import Enum

from cloud.ai.lib.python.serialization import YsonSerializable


class Mark(Enum):
    TRAIN = 'TRAIN'
    VAL = 'VAL'
    TEST = 'TEST'


class HashVersion(Enum):
    CRC_64_ISO = 'CRC_64_ISO'
    CRC_32_BZIP2 = 'CRC_32_BZIP2'
    CRC_32 = 'CRC_32'


@dataclass
class S3Object(YsonSerializable):
    endpoint: str
    bucket: str
    key: str

    def __str__(self):
        return '%s/%s/%s' % (self.endpoint, self.bucket, self.key)

    def to_https_url(self) -> str:
        return 'https://{endpoint}/{bucket}/{key}'.format(
            endpoint=self.endpoint,
            bucket=self.bucket,
            key=self.key,
        )

    @staticmethod
    def from_https_url(url) -> 'S3Object':
        url_without_scheme = url[len('https://'):]
        endpoint, bucket, key = url_without_scheme.split('/', 2)
        return S3Object(endpoint=endpoint, bucket=bucket, key=key)
