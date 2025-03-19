import logging
import os
import shlex
import subprocess

from boto3.session import Session
from botocore.exceptions import ConnectionError as BotocoreConnectionError
from botocore.exceptions import HTTPClientError
from retrying import retry

from cloud.mdb.infratests.config import InfratestConfig


def build_and_upload_dataproc_agent(config: InfratestConfig, logger: logging.Logger):
    logger.info('Building dataproc-agent')
    bin_path = build_dataproc_agent(config)
    logger.info('Uploading dataproc-agent to S3')
    url = upload_dataproc_agent(config, bin_path)
    logger.info(f'dataproc-agent uploaded to {url}')


def build_dataproc_agent(config: InfratestConfig) -> str:
    dataproc_agent_root_path = os.path.join(config.arcadia_root, 'cloud/mdb/dataproc-agent')
    cmd = 'ya make -r --target-platform=linux --keep-going'
    # compile code on distbuild ferm
    if os.environ.get('USE_DISTBUILD'):
        cmd += ' --dist --download-artifacts'
    subprocess.check_call(shlex.split(cmd), cwd=dataproc_agent_root_path)
    return os.path.join(dataproc_agent_root_path, 'cmd/dataproc-agent/dataproc-agent')


@retry(
    stop_max_attempt_number=5,
    retry_on_exception=lambda exc: isinstance(exc, (BotocoreConnectionError, HTTPClientError)),
)
def upload_dataproc_agent(config: InfratestConfig, bin_path: str) -> str:
    bucket_name = config.dataproc.agent_builds_bucket_name
    version = config.dataproc.agent_version
    dst = f'dataproc-agent-{version}'

    client = _make_resource(config)
    client.Bucket(bucket_name).upload_file(bin_path, dst, ExtraArgs={'ACL': 'public-read'})
    return f'{config.dataproc.agent_repo}/{dst}'


def _make_resource(config: InfratestConfig):
    session = Session(
        aws_access_key_id=config.provisioner_service_account.s3_access_key,
        aws_secret_access_key=config.provisioner_service_account.s3_secret_key,
    )
    return session.resource('s3', endpoint_url=f"https://{config.s3.endpoint_url}", region_name=config.s3.region_name)
