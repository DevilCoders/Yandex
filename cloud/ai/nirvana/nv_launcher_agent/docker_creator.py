from cloud.ai.nirvana.nv_launcher_agent.lib.config import Config


class PathConfig:
    def __init__(self, use_tmpfs_for_log, use_tmpfs_for_sys):
        self.use_tmpfs_for_log = use_tmpfs_for_log
        self.use_tmpfs_for_sys = use_tmpfs_for_sys

    def root_dir(self):
        return '/var/tmp/nv_job'

    def log_dir_prefix(self):
        if self.use_tmpfs_for_log:
            return 'nv_tmpfs/log/'
        else:
            return 'log/'

    def log_dir(self):
        return f'{self.root_dir()}/{self.log_dir_prefix()}'

    def sys_dir_prefix(self):
        if self.use_tmpfs_for_sys:
            return 'nv_tmpfs/sys/'
        else:
            return 'sys/'

    def sys_dir(self):
        return f'{self.root_dir()}/{self.sys_dir_prefix()}'

    def static_dir(self):
        return f'{self.sys_dir()}/static/'


class DockerCreator:
    DEFAULT_DOCKER_IMAGE = 'registry.yandex.net/ubuntu:trusty'

    def __init__(self, from_image_name, job_json_url, use_tmpfs_for_log, use_tmpfs_for_sys):
        self.config = PathConfig(use_tmpfs_for_log, use_tmpfs_for_sys)
        self.from_image_name = from_image_name
        self.job_json_url = job_json_url

    def get_dockerfile(self):
        docker_body = '\n\n'.join([
            f'WORKDIR {self.config.root_dir()}',
            self.set_envs(),
            self.make_dirs(),
            self.rm_nvidia_bin_files(),
            self.copy_files(),
            self.chmod_job_launcher_native(),
            self.run_command()
        ])

        if self.from_image_name == DockerCreator.DEFAULT_DOCKER_IMAGE:
            return self.job_launcher_default_dockerfile() + docker_body

        return f'FROM {self.from_image_name}\n\n' + docker_body

    def job_launcher_default_dockerfile(self):
        with open('./nirvana_bundle/job_launcher_no_layers.docker', 'r') as f:
            return f.readlines()

    def set_envs(self):
        return '\n'.join([
            'ENV NV_BUFFER_SIZE "_128_MB"',
            'ENV NV_JL_XMX "2080"',
            'ENV NV_JL_XX_MAX_DIRECT "128"',
            'ENV NV_JL_XX_MAX_METASPACE_SIZE "80"',
            'ENV NV_JL_XX_RESERVED_CODE_CACHE "48"',
            f'ENV NV_LOG_DIR_PREFIX "{self.config.log_dir_prefix()}"',
            'ENV NV_MTN_PROJECT_ID "4456"',
            'ENV NV_NATIVE_JOB_LAUNCHER "true"',
            f'ENV NV_SYS_DIR_PREFIX "{self.config.sys_dir_prefix()}"',
            'ENV NV_TMPFS_SANDBOX "false"',
            'ENV NV_MDS_AWS_USE_PROXIES "false"',
            'ENV NV_USE_PORTO "false"',
            'ENV PYTHONPATH "/var/tmp/nv_job/j"',
            'ENV JWD "/var/tmp/nv_job/j"',
            f'ENV NV_JOB_JSON_URL "{self.job_json_url}"'
        ])

    def make_dirs(self):
        return ' '.join([
            'RUN mkdir -p',
            self.config.sys_dir(),
            self.config.static_dir(),
            self.config.log_dir(),
            f'&& touch {self.config.sys_dir()}hosts.txt'
        ])

    def rm_nvidia_bin_files(self):
        return 'RUN rm -rf /usr/bin/nvidia*'

    def copy_files(self):
        copy_static_files = [
            f'COPY target/{file} {self.config.static_dir()}{file}'
            for file in Config.nirvana_bundle_files()
        ]
        return '\n'.join(copy_static_files)

    def chmod_job_launcher_native(self):
        return f'RUN chmod +x {self.config.static_dir()}job_launcher_native'

    def run_command(self):
        return f'CMD ["bash", "-c", "source {self.config.static_dir()}job_launcher.sh && jl_main"]'

