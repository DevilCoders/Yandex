import logging
import os
import tarfile
import shutil
from enum import Enum

from cloud.ai.nirvana.nv_launcher_agent.lib.release_manager import ReleaseManager
from cloud.ai.nirvana.nv_launcher_agent.lib.helpers import get_s3_client, read_s3_credentials
from cloud.ai.nirvana.nv_launcher_agent.lib.config import Config
from cloud.ai.nirvana.nv_launcher_agent.lib.thread_logger import ThreadLogger


class DeployStatus(Enum):
    NEED_UPDATE = 'NEED_UPDATE'
    VERSIONS_ARE_EQUAL = 'VERSIONS_ARE_EQUAL'
    UPDATING = 'UPDATING'


class Deployer:
    def __init__(self):
        self.local_release_manager = ReleaseManager(Config.get_release_info_file_path())

    def get_s3_object(self, object_key):
        ThreadLogger.info(f'Get S3 object bucket={Config.get_s3_bucket()} key={object_key}')

        s3_credentials = read_s3_credentials(Config.get_s3_credential_path())
        s3 = get_s3_client(
            Config.get_s3_endpoint_url(),
            s3_credentials['aws_access_key'],
            s3_credentials['aws_secret_key']
        )

        get_object_response = s3.get_object(Bucket=Config.get_s3_bucket(), Key=object_key)
        return get_object_response['Body'].read()

    def has_aws_keys(self):
        return os.path.exists(Config.get_s3_credential_path())

    def get_latest_release(self):
        ThreadLogger.info('Get latest release from S3')
        remote_release_json = Config.get_release_local_path('s3_release_info.json')

        payload = self.get_s3_object(Config.get_release_info_s3_path())
        ThreadLogger.info(f'Downloaded {Config.get_release_info_s3_path()} saving to {remote_release_json}')
        ThreadLogger.info(f'Payload: {payload}')
        with open(remote_release_json, 'wb') as f:
            f.write(payload)
        ThreadLogger.info(f'Saved to {remote_release_json}')
        remote_release_manager = ReleaseManager(remote_release_json)
        return remote_release_manager

    def check_for_update(self) -> DeployStatus:
        remote_release_manager = self.get_latest_release()

        remote_latest_release = remote_release_manager.get_latest()
        local_latest_release = self.local_release_manager.get_latest()

        if local_latest_release.revision_id == remote_latest_release.revision_id:
            ThreadLogger.info(f'Local and remote versions are equal. All up-to-date.')
            return DeployStatus.VERSIONS_ARE_EQUAL

        ThreadLogger.info(
            f'Local and remote versions are differ '
            f'local_revision_id={local_latest_release.revision_id} '
            f'remote_revision_id={remote_latest_release.revision_id}'
        )
        ThreadLogger.info(
            f'Downloading and unpacking new release: '
            f'release_name={remote_latest_release.release_name} '
            f'resource_name={remote_latest_release.resource_name} '
            f'info={remote_latest_release.info} '
            f'timestamp={remote_latest_release.timestamp}'
        )
        release = self.get_s3_object(remote_latest_release.s3_path)
        release_local_path = Config.get_release_local_path(remote_latest_release.resource_name)
        with open(release_local_path, 'wb') as f:
            f.write(release)

        extraction_directory = Config.get_deploy_dir()
        if os.path.exists(extraction_directory):
            shutil.rmtree(extraction_directory, ignore_errors=True)
        os.makedirs(extraction_directory)
        ThreadLogger.info(f'Extracting release to {extraction_directory}')
        with tarfile.open(release_local_path, 'r:gz') as f:
            f.extractall(extraction_directory)

        self.local_release_manager.update_release_info_from_json(remote_latest_release)
        return DeployStatus.NEED_UPDATE
