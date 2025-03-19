import os
import logging
import requests

from threading import Thread, RLock, Event
from time import sleep
from typing import Dict

from cloud.ai.nirvana.nv_launcher_agent.job_status import JobStatus
from cloud.ai.nirvana.nv_launcher_agent.lib.process.base_process import ProcessStatus, BaseProcess
from cloud.ai.nirvana.nv_launcher_agent.lib.helpers import current_timestamp, \
    get_job_launcher_stderr_json
from cloud.ai.nirvana.nv_launcher_agent.lib.retryable import retryable
from cloud.ai.nirvana.nv_launcher_agent.lib.config import Config
from cloud.ai.nirvana.nv_launcher_agent.lib.thread_logger import ThreadLogger


class JobExecutionContext:
    def __init__(self, processor_url: str, job_uid: str, yt_token: str):
        self.processor_url = processor_url
        self.yt_token = yt_token
        self.job_uid = job_uid

    def get_processor_url(self) -> str:
        return self.processor_url

    def get_job_uid(self) -> str:
        return self.job_uid

    def get_yt_token(self) -> str:
        return self.yt_token


class JobExecutor:
    def __init__(self, process_generator, context):
        self.process_generator = process_generator
        self.context = context
        process, status = next(self.process_generator)
        self.current_process: BaseProcess = process
        self.current_status: JobStatus = status
        self.job_launcher_output: Dict = None

        self.lock: RLock = RLock()
        self.stop_event = Event()
        self.main_thread = Thread(target=JobExecutor.main_loop, args=(self,))
        self.stderr = None
        self.stdout = None

    def main_loop(self):
        ThreadLogger.register_thread(os.path.join(Config.get_log_dir(), 'job_executor.stdout.log'))
        ThreadLogger.info('Starting job_executor main cycle')
        self.current_process.start()
        ThreadLogger.info(f'Job execution status: {self.status}')

        while True:
            try:
                with self.lock:
                    if self.stop_event.is_set():
                        ThreadLogger.info('Received: stop event')
                        self.current_process.kill()
                        self.current_status = JobStatus.STOPPED
                        ThreadLogger.info(f'Job execution status: {self.current_status}')
                        self.send_status_finished()
                        os._exit(1)
                        return

                process_status = self.current_process.status
                if process_status == ProcessStatus.RUNNING:
                    sleep(1)
                    continue

                with self.lock:
                    if process_status == ProcessStatus.FAILED:
                        sleep(30)
                        self.handle_job_failure()
                        return

                assert process_status == ProcessStatus.COMPLETED
                self.current_process.close_files()
                if self.current_status == JobStatus.RUNNING:
                    self.job_launcher_output = get_job_launcher_stderr_json(
                        self.current_process.exit_code(),
                        JobStatus.COMPLETED,
                        self.current_process.stderr
                    )

                job_status = self.status
                ThreadLogger.info(f'Process of execution state {job_status} was COMPLETED')

                if not job_status.is_job_in_process():
                    raise Exception("Inappropriate job status")

                with self.lock:
                    process, status = next(self.process_generator)
                    self.current_process: BaseProcess = process
                    self.current_status: JobStatus = status

                    ThreadLogger.info(f'Job execution status: {self.status}')
                    if self.current_status == JobStatus.COMPLETED:
                        self.notify_completion(self.job_launcher_output)
                        return
                    self.current_process.start()
            finally:
                if self.current_process is not None and self.current_process.status != ProcessStatus.RUNNING:
                    self.current_process.close_files()

    def handle_job_failure(self):
        ThreadLogger.error(f'Process of execution state {self.current_status} was FAILED')
        self.current_process.kill()
        self.stderr = self.current_process.stderr
        self.stdout = self.current_process.stdout

        job_output_json = get_job_launcher_stderr_json(
            self.current_process.exit_code(),
            self.current_status.value,
            self.stderr
        )

        self.current_status = JobStatus.FAILED
        ThreadLogger.info('Process stdout:')
        ThreadLogger.info(self.stdout)
        ThreadLogger.info('Process stderr:')
        ThreadLogger.info(self.stderr)

        self.notify_completion(job_output_json)

    def notify_completion(self, job_output_json):
        self.send_status_job_launcher(job_output_json)
        self.send_status_finished()

    @retryable
    def send_status_job_launcher(self, job_output_json):
        request_body = {
            'jobOutput': job_output_json,
            'createdAt': current_timestamp(),
            'name': 'USER_COMMAND_COMPLETED',
            'ticket': self.context.get_job_uid(),
            'eventType': 'JOB_OUTPUT',
            'creator': 'CloudAgent'
        }

        url = f'{self.context.get_processor_url()}/job/event/job_output'
        ThreadLogger.info(f'Send POST request to {url} with request_body={request_body}')
        response = requests.post(url,
                                 headers={'Content-Type': 'application/json',
                                          'Authorization': f'OAuth {self.context.get_yt_token()}'},
                                 json=request_body)

        return response.status_code

    @retryable
    def send_status_finished(self):
        url = f'{self.context.get_processor_url()}/cloud-agent/event/finish/{self.context.get_job_uid()}'
        ThreadLogger.info(f'Send POST request to {url}')
        response = requests.post(url,
                                 headers={'Content-Type': 'application/json',
                                          'Authorization': f'OAuth {self.context.get_yt_token()}'})

        return response.status_code

    def run(self):
        self.main_thread.start()

    def stop(self):
        ThreadLogger.info('Received stop command')
        with self.lock:
            self.stop_event.set()

    @property
    def status(self) -> JobStatus:
        with self.lock:
            return self.current_status

    def process_status_description(self):
        if self.status == JobStatus.FAILED:
            stderr = self.current_process.stderr
            if isinstance(stderr, str):
                return stderr

            return "".join(stderr)

        with self.lock:
            if self.current_process is None:
                return ''
            if isinstance(self.current_process.stdout, str):
                return self.current_process.stdout

            return "".join(self.current_process.stdout)
