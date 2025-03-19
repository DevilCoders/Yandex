import os
import tarfile

from cloud.ai.nirvana.nv_launcher_agent.lib.release_manager import ReleaseManager
from cloud.ai.nirvana.nv_launcher_agent.lib.helpers import get_s3_client
from cloud.ai.nirvana.nv_launcher_agent.lib.config import Config
from cloud.ai.nirvana.nv_launcher_agent.lib.thread_logger import ThreadLogger


def push_new_release(
    release_name,
    revision_id,
    message,
    aws_access_key,
    aws_secret_key,
    source_dir
):
    release_creator = ReleaseCreator(
        release_name=release_name,
        revision_id=revision_id,
        message=message,
        aws_access_key=aws_access_key,
        aws_secret_key=aws_secret_key,
        source_dir=source_dir
    )
    release_creator.update_release_info()
    release_creator.create_archive()
    release_creator.push_to_s3()
    release_creator.update_release_at_s3()


class ReleaseCreator:
    def __init__(
        self,
        release_name,
        revision_id,
        message,
        aws_access_key,
        aws_secret_key,
        source_dir
    ):
        self.s3 = get_s3_client(
            Config.get_s3_endpoint_url(),
            aws_access_key,
            aws_secret_key
        )

        self.source_dir = source_dir
        self.release_name = release_name
        self.resource_name = release_name + '.tar.gz'
        self.resource_path = os.path.join(self.source_dir, 'release', self.resource_name)
        self.revision_id = revision_id
        self.message = message
        self.release_manager = ReleaseManager(os.path.join(self.source_dir, 'release', 'release_info.json'))

    def update_release_info(self):
        self.release_manager.update_release_info(
            self.release_name,
            self.resource_name,
            self.revision_id,
            self.message
        )

    def create_archive(self):
        ThreadLogger.info(f'Create release archive: resource_name={self.resource_name}')
        with tarfile.open(self.resource_path, "w:gz") as tar:
            for filename in os.listdir(self.source_dir):
                if filename.startswith('.'):
                    continue

                path_to_file = os.path.join(self.source_dir, filename)
                if os.path.islink(path_to_file):
                    path_to_file = os.readlink(path_to_file)

                tar.add(path_to_file, arcname=filename)

    def push_to_s3(self):
        s3_url = Config.get_release_s3_path(self.resource_name)
        ThreadLogger.info(
            f'Push release archive to S3: '
            f'resource_name={self.resource_name}, resource_path={self.resource_path} s3_url={s3_url}'
        )
        self.s3.upload_file(
            self.resource_path,
            Config.get_s3_bucket(),
            s3_url
        )

    def update_release_at_s3(self):
        ThreadLogger.info(f'Update release info at S3')
        self.s3.upload_file(
            self.release_manager.release_info_file,
            Config.get_s3_bucket(),
            Config.get_release_info_s3_path()
        )
