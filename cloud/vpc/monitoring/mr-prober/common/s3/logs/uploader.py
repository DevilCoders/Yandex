import io
import logging
from typing import Dict, Optional

import boto3

from common.async_helper import AsyncHelper
from agent.config.models import *

__all__ = ["MrProberLogsS3Uploader", "AsyncMrProberLogsS3Uploader"]

from agent.timer import Timer


class MrProberLogsS3Uploader:
    """
    Upload Mr. Prober logs.
    Logs are stored in Yandex Object Storage (aka S3) —
    https://console.cloud.yandex.ru/folders/yc.vpc.mr-prober/storage/bucket/mr-prober-logs
    """

    def __init__(self, endpoint: str, access_key_id: str, secret_access_key: str, bucket: str, prefix: str = ""):
        self.prefix = prefix
        self.bucket = bucket

        session = boto3.session.Session()
        self.s3 = session.client(
            service_name="s3",
            endpoint_url=endpoint,
            aws_access_key_id=access_key_id,
            aws_secret_access_key=secret_access_key,
        )

    def upload_prober_log(
        self, prober: Prober, cluster: ClusterMetadata, vm_hostname: str, filename: str, content: bytes
    ) -> str:
        s3_filename = f"{self.prefix}probers/{cluster.slug}/{vm_hostname}/{prober.slug}/{filename}"
        self._upload_content(s3_filename, content)
        return s3_filename

    def upload_cluster_deployment_log(
        self, cluster: Cluster, filename: str, content: bytes, s3_extra_args: Dict[str, str]
    ) -> str:
        s3_filename = f"{self.prefix}creator/{cluster.slug}/{filename}"
        self._upload_content(s3_filename, content, s3_extra_args)
        return s3_filename

    def _upload_content(self, filename: str, content: bytes, s3_extra_args: Optional[Dict[str, str]] = None):
        extra_args = {'ContentType': 'text/plain'}
        if s3_extra_args is not None:
            extra_args.update(s3_extra_args)

        content = io.BytesIO(content)

        sending_timer = Timer()
        self.s3.upload_fileobj(content, self.bucket, filename, ExtraArgs=extra_args)
        logging.info(f"Uploaded log into S3: {filename!r}, {sending_timer.get_total_seconds():.3f} seconds elapsed")


class AsyncMrProberLogsS3Uploader(AsyncHelper, MrProberLogsS3Uploader):
    def __init__(self, *args, **kwargs):
        super().__init__(*args,**kwargs)
        self.extract_method_to_thread("upload_prober_log")
        self.extract_method_to_thread("upload_cluster_deployment_log")
