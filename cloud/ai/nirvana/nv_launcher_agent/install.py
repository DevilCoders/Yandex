import boto3
import os
import shutil
import tarfile
import logging
from argparse import ArgumentParser


logging.basicConfig(
    level=logging.DEBUG,
    format='[%(asctime)s] {%(filename)20s:%(lineno)4d} %(levelname)s - %(message)s',
    datefmt='%d-%m %H:%M:%S'
)
logger = logging.getLogger(__name__)


def get_s3_client(endpoint_url, aws_access_key, aws_secret_key):
    session = boto3.session.Session(
        aws_access_key_id=aws_access_key,
        aws_secret_access_key=aws_secret_key
    )
    return session.client(
        service_name='s3',
        endpoint_url=endpoint_url
    )


class Installer:
    def __init__(self, access_key, secret_key, registry_key, s3_release_path):
        logger.info('Installer.init')
        self.access_key = access_key
        self.secret_key = secret_key
        self.registry_key = registry_key
        self.s3_release_path = s3_release_path

        logger.info('Create s3 client')
        self.s3 = get_s3_client(
            'https://storage.yandexcloud.net',
            aws_access_key=self.access_key,
            aws_secret_key=self.secret_key
        )
        self.bucket = 'cloud-nirvana-storage'
        logger.info('Get S3 object bucket={} key={}'.format(self.bucket, self.s3_release_path))
        get_object_response = self.s3.get_object(Bucket=self.bucket, Key=self.s3_release_path)
        self.release_object = get_object_response['Body'].read()

        self.agent_dir = '/var/lib/nv_launcher_agent/'
        self.deploy_dir = os.path.join(self.agent_dir, 'deploy')
        self.release_dir = os.path.join(self.agent_dir, 'release')
        self.keys_dir = os.path.join(self.agent_dir, 'keys')

        self.release_file = os.path.join(self.release_dir, os.path.basename(self.s3_release_path))

    def enroll(self):
        self.unpack_tar_gz()
        self.save_secrets()
        self.setup_release_folder()
        self.setup_service()

    def unpack_tar_gz(self):
        if os.path.exists(self.release_dir):
            shutil.rmtree(self.release_dir, ignore_errors=True)
        os.makedirs(self.release_dir)
        logger.info("Saving release file {} to {}".format(self.s3_release_path, self.release_file))
        with open(self.release_file, 'wb') as f:
            f.write(self.release_object)

        logger.info('Unpacking tar.gz to deploy_dir = {}'.format(self.deploy_dir))
        extraction_directory = self.deploy_dir
        if os.path.exists(extraction_directory):
            shutil.rmtree(extraction_directory, ignore_errors=True)
        os.makedirs(extraction_directory)

        logger.info('Extracting release to {}'.format(extraction_directory))
        with tarfile.open(self.release_file, 'r:gz') as f:
            f.extractall(extraction_directory)

    def save_secrets(self):
        logger.info('Saving secrets to {}'.format(self.keys_dir))
        if os.path.exists(self.keys_dir):
            shutil.rmtree(self.keys_dir, ignore_errors=True)
        os.makedirs(self.keys_dir)

        logger.info('Saving aws_credentials')
        with open(os.path.join(self.keys_dir, 'aws_credentials'), 'w') as f:
            f.write('aws_access_key={}\n'.format(self.access_key))
            f.write('aws_secret_key={}\n'.format(self.secret_key))

        logger.info('Saving registry_key.json')
        with open(os.path.join(self.keys_dir, 'registry_key.json'), 'w') as f:
            f.write(self.registry_key)

    def setup_release_folder(self):
        logger.info('Setup release folder')
        src = os.path.join(self.deploy_dir, 'release', 'release_info.json')
        dst = os.path.join(self.release_dir, 'release_info.json')
        logger.info('Copying file release_info.json from={} to={}'.format(src, dst))
        shutil.copy(src, dst)

    @staticmethod
    def _print_and_exec(command):
        logger.info('Exec: {}'.format(command))
        os.system(command)

    def setup_service(self):
        logger.info('Setup nv-launcher-proxy.service')
        service_name = 'nv-launcher-proxy.service'
        src = os.path.join(self.deploy_dir, service_name)
        dst = os.path.join('/etc/systemd/system/', service_name)
        logger.info('Copy service file')
        shutil.copy(src, dst)

        Installer._print_and_exec("sudo chmod 664 {}".format(dst))
        Installer._print_and_exec("sudo systemctl daemon-reload")
        Installer._print_and_exec("sudo systemctl enable {}".format(service_name))
        Installer._print_and_exec("sudo systemctl start  {}".format(service_name))


def main():
    parser = ArgumentParser()
    parser.add_argument('--access-key', required=True)
    parser.add_argument('--secret-key', required=True)
    parser.add_argument('--registry-key', required=True)
    parser.add_argument('--s3-release-path', required=True)
    args = parser.parse_args()
    installer = Installer(args.access_key, args.secret_key, args.registry_key, args.s3_release_path)
    installer.enroll()


if __name__ == '__main__':
    main()
