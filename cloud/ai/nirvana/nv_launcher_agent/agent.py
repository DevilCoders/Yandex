from flask import Flask, request, json, Response

from argparse import ArgumentParser

from cloud.ai.nirvana.nv_launcher_agent.lib.helpers import HttpCode
from cloud.ai.nirvana.nv_launcher_agent.job_manager import \
    HostJobManager, UnknownJobException, AlreadyStoppedJobException
from cloud.ai.nirvana.nv_launcher_agent.job_status import JobStatus

from cloud.ai.nirvana.nv_launcher_agent.lib.config import Config
from cloud.ai.nirvana.nv_launcher_agent.lib.key_management_service import KeyManagementService

class JobAgentService:
    @staticmethod
    def get_options(parser: ArgumentParser):
        parser.add_argument('--working-dir', type=str, required=True)
        parser.add_argument('--layers-dir', type=str, required=True)
        parser.add_argument('--port', type=int, default=5000)
        return parser

    def __init__(self, args):
        self.app = Flask(__name__)
        self.job_manager = HostJobManager(
            working_dir=args.working_dir,
            layers_dir=args.layers_dir
        )
        self.key_management_service = KeyManagementService(
            Config.get_kms_key_id(),
            Config.get_cloud_service_account_key_id(),
            Config.get_cloud_service_account_id(),
            Config.get_cloud_service_account_private_key_pem_file()
        )

        self.app.add_url_rule('/api/run', 'run', self.run_job, methods=['POST'])
        self.app.add_url_rule('/api/status/<job_id>', 'status', self.job_status, methods=['GET'])
        self.app.add_url_rule('/api/stop/<job_id>', 'stop', self.stop_job, methods=['POST'])
        self.app.run(host='::', port=args.port)

    def run_job(self):
        data = json.loads(request.get_data())
        print(data['envVariables'])
        print(data['executorProperties'])
        #data['executorProperties'] = json.loads(data['executorProperties'])

        secrets = data['secrets']
        encoded_yt_token = secrets['ytToken']
        encoded_mds_static_access_key = secrets['mdsAccessKey']
        encoded_mds_static_secret_key = secrets['mdsSecretKey']
        encoded_mds_service_id = secrets['mdsServiceId']
        encoded_aws_access_key = secrets['awsAccessKey']
        encoded_aws_secret_key = secrets['awsSecretKey']
        encoded_docker_registry_key = secrets['cloudRegistryKey']

        decoded_yt_token = self.key_management_service.decrypt(encoded_yt_token)
        decoded_mds_static_access_key = self.key_management_service.decrypt(encoded_mds_static_access_key)
        decoded_mds_static_secret_key = self.key_management_service.decrypt(encoded_mds_static_secret_key)
        decoded_mds_service_id = self.key_management_service.decrypt(encoded_mds_service_id)

        decoded_aws_access_key = self.key_management_service.decrypt(encoded_aws_access_key)
        decoded_aws_secret_key = self.key_management_service.decrypt(encoded_aws_secret_key)
        decoded_docker_registry_key = self.key_management_service.decrypt(encoded_docker_registry_key)

        with open(Config.get_s3_credential_path(), 'w') as f:
            f.write('aws_access_key={}\n'.format(decoded_aws_access_key))
            f.write('aws_secret_key={}\n'.format(decoded_aws_secret_key))

        with open(Config.get_docker_registry_key_path(), 'w') as f:
            f.write(decoded_docker_registry_key)

        status = self.job_manager.run_job(
            envVariables=data['envVariables'],
            executorProperties=data['executorProperties'],
            layers=data['layers'],
            yt_token=decoded_yt_token,
            mds_static_access_key=decoded_mds_static_access_key,
            mds_static_secret_key=decoded_mds_static_secret_key,
            mds_service_id=decoded_mds_service_id,
            nirvana_bundle_info=data['resources'],
            cached_layer_name=data['executorProperties'].get('cached_layer_name')
        )

        if status == JobStatus.ACCEPTED:
            return self.job_manager.get_active_job_id(), HttpCode.OK
        else:
            return 'Host already has a job', HttpCode.BAD_REQUEST

    def job_status(self, job_id):
        try:
            status_response = self.job_manager.get_job_status(job_id)
        except UnknownJobException as e:
            return Response(e.message, status=HttpCode.BAD_REQUEST)

        return json.dumps(status_response.to_json()), HttpCode.OK

    def stop_job(self, job_id):
        try:
            self.job_manager.stop_job(job_id)
        except UnknownJobException as e:
            return Response(e.message, status=HttpCode.BAD_REQUEST)
        except AlreadyStoppedJobException as e:
            return Response(e.message, status=HttpCode.BAD_REQUEST)
        return 'Job stopped', HttpCode.OK
