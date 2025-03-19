import hashlib
import json
import logging
import os
from shutil import rmtree
from typing import List
from uuid import uuid4

import requests
from cloud.ai.nirvana.nv_launcher_agent.docker_cache import DockerCacheService
from cloud.ai.nirvana.nv_launcher_agent.docker_creator import DockerCreator
from cloud.ai.nirvana.nv_launcher_agent.job_executor import JobExecutor, JobExecutionContext
from cloud.ai.nirvana.nv_launcher_agent.job_status import JobStatus
from cloud.ai.nirvana.nv_launcher_agent.layer_manager import LayerManager
from cloud.ai.nirvana.nv_launcher_agent.lib.config import Config
from cloud.ai.nirvana.nv_launcher_agent.lib.helpers import get_s3_client, read_s3_credentials
from cloud.ai.nirvana.nv_launcher_agent.lib.helpers import merge_layers_args
from cloud.ai.nirvana.nv_launcher_agent.lib.process.base_process import ProcessStatus
from cloud.ai.nirvana.nv_launcher_agent.lib.process.cli_process import CliProcess
from cloud.ai.nirvana.nv_launcher_agent.lib.process.function_process import FunctionProcess
from cloud.ai.nirvana.nv_launcher_agent.lib.process.job_process import JobProcess
from cloud.ai.nirvana.nv_launcher_agent.lib.retryable import retryable
from cloud.ai.nirvana.nv_launcher_agent.lib.thread_logger import ThreadLogger
from cloud.ai.nirvana.nv_launcher_agent.secrets import Secrets


class Job:
    def __init__(
        self,
        envVariables: dict,
        executorProperties: dict,
        layers: List[str],
        secrets: Secrets,
        cached_base_image_name: str,
        layer_manager: LayerManager,
        parent_working_dir: str,
        nirvana_bundle_info: str
    ):
        self.envVariables = envVariables
        self.executorProperties = executorProperties

        self.secrets: Secrets = secrets

        self.layers = layers
        self.layer_manager: LayerManager = layer_manager
        self.local_layers_paths = []

        self.parent_working_dir = parent_working_dir
        self.nirvana_bundle_info = nirvana_bundle_info

        self.__job_uid: str = executorProperties['jobUid']
        self.working_dir = os.path.join(parent_working_dir, self.__job_uid)
        self.log_dir = os.path.join(self.working_dir, 'logs')

        self.job_launcher_image_name = "job_launcher_image_" + self.__job_uid.lower().replace("-", "").replace("_", "")
        base_docker_image = executorProperties['dockerImagePath']
        if base_docker_image is not None:
            self.cached_base_image_name = base_docker_image
            self.base_image_name = base_docker_image
        else:
            self.cached_base_image_name = cached_base_image_name

        if base_docker_image is None:
            if self.cached_base_image_name is not None:
                logging.info(f'Will use cached layer {self.cached_base_image_name}')
                self.base_image_name = f'{Config.get_cr_layer_repo()}/{self.cached_base_image_name}:latest'
            else:
                logging.info(f'No cached layer detected. Will build new one.')
                self.base_image_name = DockerCreator.DEFAULT_DOCKER_IMAGE if len(self.layers) == 0 else str(uuid4())

        self.docker_cache_service = DockerCacheService(Config.get_cr_layer_repo(), secrets.registry_key_string)

        self.prepare_environment()
        self.job_executor: JobExecutor = self.create_job_executor()
        ThreadLogger.info(f'Created job with uid={self.__job_uid}')

    @property
    def id(self):
        return self.__job_uid

    @property
    def processor_url(self):
        return self.executorProperties['processorUrl']

    @property
    def gpu_count(self):
        return self.executorProperties['gpuCount']

    def create_job_executor(self) -> JobExecutor:
        ThreadLogger.info('Creating job executor')
        downloading_bundle_process = self.create_downloading_bundle_process()
        pulling_porto_process = self.create_pull_layers_process()
        converting_porto_process = self.create_convert_layers_to_docker_image_process()
        put_image_process = self.create_upload_image_process()
        get_cached_image_process = self.create_get_cached_image_process()
        building_image_process = self.create_build_image_process()
        job_subprocess = self.create_run_operation_process()

        def process_generator():
            yield downloading_bundle_process, JobStatus.DOWNLOADING_NIRVANA_BUNDLE
            if self.cached_base_image_name is None:
                yield pulling_porto_process, JobStatus.PULLING_PORTO_LAYERS
                yield converting_porto_process, JobStatus.CONVERTING_PORTO_LAYERS
                yield put_image_process, JobStatus.PUT_DOCKER_IMAGE_TO_CACHE
            else:
                yield get_cached_image_process, JobStatus.GET_CACHED_DOCKER_IMAGE

                if get_cached_image_process.status != ProcessStatus.COMPLETED:
                    logging.warning("Unable to pull cached image from registry. Fallback to manual building.")
                    yield pulling_porto_process, JobStatus.PULLING_PORTO_LAYERS
                    yield converting_porto_process, JobStatus.CONVERTING_PORTO_LAYERS
                    yield put_image_process, JobStatus.PUT_DOCKER_IMAGE_TO_CACHE

            yield building_image_process, JobStatus.BUILDING_IMAGE
            yield job_subprocess, JobStatus.RUNNING
            yield None, JobStatus.COMPLETED
        return JobExecutor(
            process_generator=process_generator(),
            context=JobExecutionContext(self.executorProperties['processorUrl'], self.executorProperties['jobUid'], self.secrets.yt_token)
        )

    def create_downloading_bundle_process(self):
        ThreadLogger.info('Create downloading bundle process')

        download_bundle_process = FunctionProcess(
            name='download_bundle',
            log_dir=self.log_dir,
            target=self.download_nirvana_bundle,
        )

        ThreadLogger.info('Successfully created downloading bundle process')

        return download_bundle_process

    def create_pull_layers_process(self):
        ThreadLogger.info('Create pull porto layers process')
        if self.base_image_name == DockerCreator.DEFAULT_DOCKER_IMAGE:
            ThreadLogger.info('Porto to docker: default image, exit')
            return

        pull_layers_process = FunctionProcess(
            name='pull_layers',
            log_dir=self.log_dir,
            target=self.pull_layers_from_yt,
        )

        ThreadLogger.info('Successfully created pull porto layers process')

        return pull_layers_process

    def create_convert_layers_to_docker_image_process(self):
        ThreadLogger.info('Create merge porto layers & docker import process')
        if self.base_image_name == DockerCreator.DEFAULT_DOCKER_IMAGE:
            ThreadLogger.info('Porto to docker: default image, exit')
            return

        self.__collect_local_layers_paths()

        ThreadLogger.info(f'Merge layers: {", ".join(self.local_layers_paths)}')
        merge_args = merge_layers_args(self.local_layers_paths, self.layer_manager.layers_dir, self.base_image_name)
        ThreadLogger.info(f'Porto to docker process args: {merge_args}')
        import_image_process = CliProcess(
            name='merge_images',
            args=merge_args,
            log_dir=self.log_dir,
            shell=True
        )

        ThreadLogger.info('Successfully created merge porto layers & docker import process')

        return import_image_process

    def create_get_cached_image_process(self):
        ThreadLogger.info('Create get image from cache process')

        def run_download_image(image_name, service):
            ThreadLogger.info(f'Pulling image {image_name}')
            service.download_layer(image_name)
            ThreadLogger.info(f'Pulled image {image_name}')

        get_cached_image_process = FunctionProcess(
            name='get_cached_image',
            args=(self.base_image_name, self.docker_cache_service),
            log_dir=self.log_dir,
            target=run_download_image
        )

        ThreadLogger.info('Successfully created get image from cache process')

        return get_cached_image_process

    def create_upload_image_process(self):
        ThreadLogger.info('Create upload image to cache process')
        porto_layers_hash = self.get_porto_layers_hash()

        def notify_processor_cache(remote_image_name):
            logging.info(f'Notifying job processor about caching layer {remote_image_name} with hash(id)={porto_layers_hash}')
            self.send_cached_layer_info(porto_layers_hash, remote_image_name)

        def run_upload_image(image_name, service):
            ThreadLogger.info(f'Pushing image {image_name} to registry')
            try:
                remote_image_name = service.upload_layer(image_name, porto_layers_hash)
                ThreadLogger.info(f'Pushed layer: {remote_image_name}')
                notify_processor_cache(remote_image_name)
            except:
                ThreadLogger.info("Unable to push image to registry! Job processor was not notified.")

        upload_image_process = FunctionProcess(
            name='upload_image_process',
            args=(self.base_image_name, self.docker_cache_service),
            log_dir=self.log_dir,
            target=run_upload_image
        )

        ThreadLogger.info('Successfully created upload image to cache process')

        return upload_image_process

    def create_build_image_process(self):
        ThreadLogger.info('Creating building docker image process')
        docker_build_command = ['docker', 'build', '-t', self.job_launcher_image_name, '.']
        ThreadLogger.info(f'Docker image process args: {docker_build_command}')
        build_image_process = CliProcess(
            name='docker_build',
            args=docker_build_command,
            cwd=self.working_dir,
            log_dir=self.log_dir
        )

        ThreadLogger.info('Successfully created building docker image process')

        return build_image_process

    def create_run_operation_process(self):
        ThreadLogger.info('Create run operation process')
        docker_run_command = ['docker', 'run',
                              '-e', f'NV_YT_TOKEN={self.secrets.yt_token}',
                              '--mount', 'type=bind,source=/mnt,target=/mnt,bind-propagation=shared',
                              '-e', f'NV_MDS_STATIC_ACCESS_KEY={self.secrets.mds_static_access_key}',
                              '-e', f'NV_MDS_STATIC_SECRET_KEY={self.secrets.mds_static_secret_key}',
                              '-e', f'NV_MDS_SERVICE_ID={self.secrets.mds_service_id}',
                              '--network', 'host',
                              '--ipc=host']
        if self.gpu_count > 0:
            docker_run_command.extend([
                '--runtime=nvidia',
                '-e', 'NVIDIA_DRIVER_CAPABILITIES=all',
                '--gpus', 'all',
                '--privileged'
            ])

        docker_run_command.append(self.job_launcher_image_name)
        ThreadLogger.info(f'Run job process args: {docker_run_command}')
        run_job_process = JobProcess(
            name='job',
            args=docker_run_command,
            cwd=self.working_dir,
            log_dir=self.log_dir,
            docker_name=self.job_launcher_image_name
        )

        ThreadLogger.info('Successfully created run operation process')

        return run_job_process

    def pull_layers_from_yt(self):
        for layer in self.layers:
            self.layer_manager.get_layer_by_yt_path(layer, yt_token=self.secrets.yt_token, proxy='hahn')

    def download_nirvana_bundle(self):
        ThreadLogger.info("Downloading bundle...")
        s3_credentials = read_s3_credentials(Config.get_s3_credential_path())
        s3 = get_s3_client(
            Config.get_s3_endpoint_url(),
            s3_credentials['aws_access_key'],
            s3_credentials['aws_secret_key']
        )
        bucket = Config.get_s3_bucket()
        nirvana_bundle_save_path_directory = os.path.join(self.working_dir, 'target')
        if not os.path.exists(nirvana_bundle_save_path_directory):
            os.makedirs(nirvana_bundle_save_path_directory)

        for bundle_item in self.nirvana_bundle_info:
            retries = 0
            while retries <= 5:
                retries += 1
                try:
                    ThreadLogger.info('Bundle item={}'.format(bundle_item))
                    bundle_name_parts = bundle_item.split(".")
                    bundle_name_parts = bundle_name_parts[:-1]
                    file_without_checksum = ".".join(bundle_name_parts)
                    ThreadLogger.info('File={}'.format(file_without_checksum))
                    dest_file_path = nirvana_bundle_save_path_directory + "/" + file_without_checksum
                    s3_path = "nirvana_bundle/" + bundle_item
                    ThreadLogger.info('Get S3 object bucket={} key={}'.format(bucket, s3_path))
                    get_object_response = s3.get_object(Bucket=bucket, Key=s3_path)
                    bundle_object = get_object_response['Body'].read()

                    with open(dest_file_path, 'wb') as f:
                        f.write(bundle_object)

                    break
                except Exception as e:
                    ThreadLogger.info(f'Got exception during download! Message: {e}')
                    continue


    def __collect_local_layers_paths(self):
        for layer in self.layers:
            layer_name = os.path.basename(layer)
            local_layer_path = os.path.join(self.layer_manager.layers_dir, layer_name)
            self.local_layers_paths.append(local_layer_path)

        # reverse layers, base layer first
        self.local_layers_paths = self.local_layers_paths[::-1]

    def create_dockerfile(self):
        ThreadLogger.info('Preparing environment: creating job_launcher docker file')
        dockerfile = DockerCreator(
            self.base_image_name,
            job_json_url=self.envVariables['job_json_url'],
            use_tmpfs_for_log=self.envVariables['useTmpfsForLog'],
            use_tmpfs_for_sys=self.envVariables['useTmpfsForSys']
        ).get_dockerfile()

        dst_dockerfile = os.path.join(self.working_dir, 'Dockerfile')
        with open(dst_dockerfile, 'w') as f:
            f.write(dockerfile)

    def prepare_environment(self):
        ThreadLogger.info('Preparing task environment')
        if os.path.exists(self.working_dir):
            rmtree(self.working_dir)
        os.makedirs(self.working_dir)
        os.makedirs(self.log_dir)

        docker_target_dir = os.path.join(self.working_dir, 'target')
        os.makedirs(docker_target_dir)

        self.create_dockerfile()

    def get_porto_layers_hash(self):
        layers_ids = list(map(lambda p: p.split("/")[-1], self.local_layers_paths))
        layers_ids = list(sorted(layers_ids))
        return hashlib.sha256("_".join(layers_ids).encode("utf-8")).hexdigest()

    @retryable
    def send_cached_layer_info(self, image_hash, image_name):
        url = f'{self.processor_url}/cloud-agent/event/cache-image?imageHash={image_hash}&imageName={image_name}'
        ThreadLogger.info(f'Send POST request to {url}')
        response = requests.post(
            url,
            headers={'Content-Type': 'application/json',
                     'Authorization': f'OAuth {self.secrets.yt_token}'}
        )

        return response.status_code

    def run(self):
        self.job_executor.run()

    @property
    def status(self):
        return self.job_executor.status

    @property
    def status_description(self):
        return self.job_executor.process_status_description()

    def stop(self):
        self.job_executor.stop()
