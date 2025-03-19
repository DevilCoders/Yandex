import os
from typing import List, Dict

from cloud.ai.nirvana.nv_launcher_agent.job import Job
from cloud.ai.nirvana.nv_launcher_agent.job_status import JobStatus
from cloud.ai.nirvana.nv_launcher_agent.lib.config import Config
from cloud.ai.nirvana.nv_launcher_agent.lib.thread_logger import ThreadLogger
from cloud.ai.nirvana.nv_launcher_agent.layer_manager import LayerManager
from cloud.ai.nirvana.nv_launcher_agent.secrets import Secrets


class UnknownJobException(Exception):
    def __init__(self, message):
        super(UnknownJobException, self).__init__()
        self.message = message


class AlreadyStoppedJobException(Exception):
    def __init__(self, message):
        super(AlreadyStoppedJobException, self).__init__()
        self.message = message


class JobStatusResponse:
    def __init__(self, status: JobStatus, description: str):
        self.status = status
        self.description = description

    def to_json(self):
        return {
            'status': self.status.value,
            'description': self.description
        }


class HostJobManager:
    def __init__(self, working_dir, layers_dir):
        ThreadLogger.info(
            f'HostJobManager.init '
            f'working_dir={working_dir}, '
            f'layers_dir={layers_dir}'
        )
        self.working_dir = working_dir
        self.layer_manager = LayerManager(layers_dir=layers_dir)

        if not os.path.exists(self.working_dir):
            os.makedirs(self.working_dir)

        self.finished_jobs: Dict[str, Job] = {}
        self.active_job = None

    @property
    def has_active_job(self):
        return self.active_job is not None

    def run_job(
        self,
        envVariables: dict,
        executorProperties: dict,
        layers: List[str],
        yt_token: str,
        mds_static_access_key: str,
        mds_static_secret_key: str,
        mds_service_id: str,
        nirvana_bundle_info: List[str],
        cached_layer_name: str = None
    ):
        ThreadLogger.info('JobManager: Run job')

        if self.has_active_job:
            self.get_job_status(self.active_job.id)

        if self.has_active_job:
            ThreadLogger.warning('JobManager: Job declined, host already has active job')
            return JobStatus.DECLINED

        secrets = Secrets(
            yt_token=yt_token,
            mds_static_access_key=mds_static_access_key,
            mds_static_secret_key=mds_static_secret_key,
            mds_service_id=mds_service_id,
            registry_key_path=Config.get_docker_registry_key_path()
        )
        self.active_job: Job = Job(
            envVariables=envVariables,
            executorProperties=executorProperties,
            layers=layers,
            secrets=secrets,
            cached_base_image_name=cached_layer_name,  # name like this: snazzy-heliotrope-stingray-1234 or None
            layer_manager=self.layer_manager,
            parent_working_dir=self.working_dir,
            nirvana_bundle_info=nirvana_bundle_info
        )
        self.active_job.run()
        ThreadLogger.warning('JobManager: Job accepted and run')

        return JobStatus.ACCEPTED

    def get_active_job_id(self):
        return self.active_job.id

    def mark_active_as_finished(self):
        self.finished_jobs[self.active_job.id] = self.active_job
        self.active_job = None

    def get_job_status(self, job_id: str) -> JobStatusResponse:
        ThreadLogger.info('JobManager: get_job_status')
        if job_id in self.finished_jobs:
            return JobStatusResponse(
                status=self.finished_jobs[job_id].status,
                description='Finished'
            )

        if self.has_active_job and self.active_job.id == job_id:
            status = self.active_job.status
            status_description = self.active_job.status_description
            if status == JobStatus.COMPLETED or status == JobStatus.FAILED:
                self.mark_active_as_finished()
            return JobStatusResponse(status=status, description=status_description)

        raise UnknownJobException(f'Unknown job_id: {job_id}')

    def stop_job(self, job_id: str):
        ThreadLogger.info('JobManager: stop job')
        if job_id in self.finished_jobs:
            ThreadLogger.error('JobManager: Attempt to stop already stopped job')
            raise AlreadyStoppedJobException(f'Job with id: {job_id} already stopped')

        if self.has_active_job and self.active_job.id == job_id:
            ThreadLogger.info('JobManager: stopping job')
            self.active_job.stop()
            self.mark_active_as_finished()
            return

        ThreadLogger.info(f'JobManager: stopping job -- unknown job_id {job_id}')
        raise UnknownJobException(f'Unknown job_id: {job_id}')
