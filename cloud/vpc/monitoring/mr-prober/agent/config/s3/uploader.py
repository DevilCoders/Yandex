import io
from typing import Union, List, Dict, Tuple, Set

import boto3
import botocore.exceptions

from agent.config.models import *

__all__ = ["AgentConfigS3Uploader", "AgentConfigS3DiffBuilder"]


class AgentConfigS3Uploader:
    """
    Uploads configuration of clusters and probers.
    Configuration is stored in Yandex Object Storage (aka S3) —
    https://console.cloud.yandex.ru/folders/yc.vpc.mr-prober/storage
    """

    def __init__(
        self, endpoint: str, access_key_id: str, secret_access_key: str, bucket: str, prefix: str = "",
        connect_timeout: Union[int, float] = 10, retry_attempts: int = 1,
    ):
        self.prefix = prefix
        self.bucket = bucket

        session = boto3.session.Session()
        self.s3 = session.client(
            service_name="s3",
            endpoint_url=endpoint,
            aws_access_key_id=access_key_id,
            aws_secret_access_key=secret_access_key,
            config=boto3.session.Config(connect_timeout=connect_timeout, retries={"max_attempts": retry_attempts})
        )

    def upload_prober_config(self, prober: Prober):
        self._upload_content(f"{self.prefix}probers/{prober.id}/prober.json", prober.json().encode())

    def upload_prober_file(self, prober_id: int, prober_file_id: int, content: bytes):
        s3_filename = f"{self.prefix}probers/{prober_id}/files/{prober_file_id}"
        self._upload_content(s3_filename, content)

    def upload_cluster_config(self, cluster: Cluster):
        self._upload_content(f"{self.prefix}clusters/{cluster.id}/cluster.json", cluster.json().encode())

    def _upload_content(self, filename: str, content: bytes):
        content = io.BytesIO(content)
        self.s3.upload_fileobj(content, self.bucket, filename)


class AgentConfigS3DiffBuilder(AgentConfigS3Uploader):
    """
    This class "mocks" content uploading and calculates diff between actual and expected content.
    """

    def __init__(
        self, endpoint: str, access_key_id: str, secret_access_key: str, bucket: str, prefix: str = "",
        connect_timeout: Union[int, float] = 10, retry_attempts: int = 1,
    ):
        super().__init__(
            endpoint, access_key_id, secret_access_key, bucket, prefix,
            connect_timeout, retry_attempts
        )
        self._unchanged_files: Set[str] = set()
        self._changed_files: Dict[str, Tuple[bytes, bytes]] = {}

    def _upload_content(self, filename: str, content: bytes):
        # Read current file's content from S3 and compare it with passed `content`.
        content_object = io.BytesIO()
        try:
            self.s3.download_fileobj(self.bucket, filename, content_object)
        except botocore.exceptions.ClientError as e:
            if e.response["Error"]["Code"] == "404":
                # File doesn't exist, assume it as a new one
                self._changed_files[filename] = (b"", content)
                return
            else:
                raise
        content_object.seek(0)

        current_content = content_object.read()

        # Put the file in one of two buckets: with unchanged on changed files
        if current_content == content:
            self._unchanged_files.add(filename)
        else:
            self._changed_files[filename] = (current_content, content)

    def get_unchanged_files(self) -> List[str]:
        return sorted(list(self._unchanged_files))

    def get_changed_files(self) -> Dict[str, Tuple[bytes, bytes]]:
        return self._changed_files
