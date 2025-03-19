import os


class Config:
    PROJECT_DIR = os.path.abspath(os.path.dirname(os.path.dirname(__file__)))

    @staticmethod
    def nirvana_bundle_files():
        return [
            'iss_hook_status',
            'jdkrun',
            'job_context.ftl',
            'job_context_base.ftl',
            'job_launcher.sh',
            'job_launcher.vmoptions_short',
            'job_launcher_locked_files',
            'job_launcher_native',
            'pylib-__init__.py',
            'pylib-job_context.py',
            'pylib-snapshot.py'
        ]

    @staticmethod
    def get_docker_images_cache_dir():
        return 'cloud-nirvana-images-cache'

    @staticmethod
    def get_cr_common_repo_prefix():
        return 'cr.yandex/crppns4pq490jrka0sth/'

    @staticmethod
    def get_cr_layer_repo():
        return Config.get_cr_common_repo_prefix() + Config.get_docker_images_cache_dir()

    @staticmethod
    def get_working_dir():
        return '/var/lib/nv_launcher_agent'

    @staticmethod
    def get_project_dir():
        return Config.PROJECT_DIR

    @staticmethod
    def set_project_dir(dir_name):
        Config.PROJECT_DIR = dir_name

    @staticmethod
    def get_project_dir_name():
        return os.path.basename(Config.get_project_dir())

    @staticmethod
    def get_release_dir():
        return os.path.join(Config.get_working_dir(), 'release')

    @staticmethod
    def get_release_info_file_name():
        return 'release_info.json'

    @staticmethod
    def get_release_info_file_path():
        return Config.get_release_local_path(Config.get_release_info_file_name())

    @staticmethod
    def get_release_local_path(resource_name):
        return os.path.join(Config.get_release_dir(), resource_name)

    @staticmethod
    def get_release_s3_path(resource_name):
        return f'nv_launcher_agent/revisions/{resource_name}'

    @staticmethod
    def get_release_info_s3_path():
        return Config.get_release_s3_path(Config.get_release_info_file_name())

    @staticmethod
    def get_s3_endpoint_url():
        return 'https://storage.yandexcloud.net'

    @staticmethod
    def get_s3_bucket():
        return 'cloud-nirvana-storage'

    @staticmethod
    def get_log_dir():
        return os.path.join(Config.get_working_dir(), 'log')

    @staticmethod
    def get_deploy_dir():
        return os.path.join(Config.get_working_dir(), 'deploy')

    @staticmethod
    def get_key_path():
        return os.path.join(Config.get_working_dir(), 'keys')

    @staticmethod
    def get_s3_credential_path():
        return os.path.join(Config.get_key_path(), 'aws_credentials')

    @staticmethod
    def get_docker_registry_key_path():
        return os.path.join(Config.get_key_path(), 'registry_key.json')

    @staticmethod
    def get_kms_key_id():
        return 'abjrifri9fqbedk485bk'

    @staticmethod
    def get_cloud_service_account_id():
        return 'ajemird1pjfeblnrf14s'

    @staticmethod
    def get_cloud_service_account_key_id():
        return 'ajep2f0kv8ldt7ctsvo6'

    @staticmethod
    def get_cloud_service_account_private_key_pem_file():
        return os.path.join(Config.get_key_path(), 'cloud_nirvana_private_key')

    @staticmethod
    def get_nirvana_bundle_directory_path():
        return "/var/lib/nv_launcher_agent/nirvana_bundle"
