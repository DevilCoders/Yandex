import json
from argparse import ArgumentParser

import requests
from flask import Flask, request

from cloud.ai.nirvana.nv_launcher_agent.controller import AgentController
from cloud.ai.nirvana.nv_launcher_agent.deployer import DeployStatus
from cloud.ai.nirvana.nv_launcher_agent.job_manager import JobStatusResponse
from cloud.ai.nirvana.nv_launcher_agent.job_status import JobStatus
from cloud.ai.nirvana.nv_launcher_agent.lib.config import Config
from cloud.ai.nirvana.nv_launcher_agent.lib.helpers import HttpCode
from cloud.ai.nirvana.nv_launcher_agent.lib.thread_logger import ThreadLogger


class ProxyService:
    @staticmethod
    def get_options(parser: ArgumentParser):
        parser.add_argument('--agent-host', default='[::]')
        parser.add_argument('--port', type=int, default=5000, required=True)
        parser.add_argument('--agent-port', type=int, default=7842, required=True)
        parser.add_argument('--debug', type=bool, default=False)
        return parser

    def __init__(self, args):
        self.host = args.agent_host
        self.port = args.port
        self.dst_port = args.agent_port

        self.agent_controller = AgentController(self.dst_port, Config.get_log_dir(), args.debug)

        self.app = Flask(__name__)
        self.app.add_url_rule('/api/run', 'run', self.run_job, methods=['POST'])
        self.app.add_url_rule('/api/status/<job_id>', 'status', self.job_status, methods=['GET'])
        self.app.add_url_rule('/api/stop/<job_id>', 'stop', self.stop_job, methods=['POST'])
        self.app.run(host='::', port=self.port)

    def agent_alive(self):
        return self.agent_controller.agent_alive()

    def get_base_url(self, endpoint):
        return f'http://{self.host}:{self.dst_port}{endpoint}'

    @staticmethod
    def answer_unavailable(message):
        job_status = JobStatusResponse(JobStatus.AGENT_UNAVAILABLE, message)
        return json.dumps(job_status.to_json()), HttpCode.UNAVAILABLE

    def post(self, endpoint, data=None, body=None):
        if not self.agent_alive():
            return self.answer_unavailable("Agent is down")

        response = requests.post(self.get_base_url(endpoint), data=data, json=body)
        return response.content, response.status_code, response.headers.items()

    def get(self, endpoint, data=None, body=None):
        if not self.agent_alive():
            return self.answer_unavailable("Agent is down")

        response = requests.get(self.get_base_url(endpoint), data=data, json=body)
        return response.content, response.status_code, response.headers.items()

    def run_job(self):
        data = json.loads(request.get_data())
        ThreadLogger.info('Get run job request')

        if self.agent_controller.check_for_update() == DeployStatus.NEED_UPDATE:
            self.agent_controller.update()
            return self.answer_unavailable('Unavailable: The agent is being updated')

        return self.post(endpoint='/api/run', data=json.dumps(data))

    def job_status(self, job_id):
        self.agent_alive()
        return self.get(f'/api/status/{job_id}')

    def stop_job(self, job_id):
        self.agent_alive()
        return self.post(f'/api/stop/{job_id}')
