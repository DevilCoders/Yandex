import unittest
from unittest import TestCase

import json
import os
import tempfile
import time
from collections import namedtuple

from cloud.ai.nirvana.nv_launcher_agent.docker_creator import DockerCreator
from cloud.ai.nirvana.nv_launcher_agent.job import Job
from cloud.ai.nirvana.nv_launcher_agent.job_status import JobStatus
from cloud.ai.nirvana.nv_launcher_agent.job_manager import HostJobManager
from cloud.ai.nirvana.nv_launcher_agent.layer_manager import LayerManager
from cloud.ai.nirvana.nv_launcher_agent.secrets import Secrets
from cloud.ai.nirvana.nv_launcher_agent.lib.helpers import parse_job_uid

RunJobRequest = namedtuple(
    'RunJobRequest',
    [
        'job_json',
        'layers',
        'secrets'
    ]
)


class Test(TestCase):
    def test_layer_manager(self):
        yt_file_example = '//home/mlcloud/d-kruchinin/cft_typos/readme.txt'
        yt_token = self.__read_token('~/.keys/robot_cloud_ai_yt_token')

        with tempfile.TemporaryDirectory() as tmp_dir:
            layer_manager = LayerManager(tmp_dir)
            local_file = layer_manager.get_layer_by_yt_path(yt_file_example, yt_token=yt_token, proxy='hahn')

            with open(local_file, 'r') as f:
                lines = f.readlines()
                expected_lines = [
                    'TaskType:multiclass',
                    'IsInternalDataset:False',
                    'NumberOfFeatures:3',
                    'NumberOfCategoryFeatures:0',
                    'NumberOfNumericFeatures:0',
                    'NumberOfTextFeatures:2',
                    'NumberOfInstances:1991104',
                    '',
                    'Goal: predict typos in names, labels: 0 -- no typo, 1 -- contain typos, 2 -- completely wrong.',
                    'Also data contain the correct name in 4-th column for objects with label=1.',
                    'URL:https://datasouls.com/c/cft-contest/description'
                ]
                for line, expected_line in zip(lines, expected_lines):
                    assert line == expected_line + '\n'

    @staticmethod
    def __read_token(file):
        with open(os.path.expanduser(file), 'r') as f:
            return f.read()[:-1]

    def get_run_job_request(self):
        with open('resource/echo_to_tsv_job.json', 'r') as f:
            job_json = json.load(f)['job_json']
        layers = []

        secrets = Secrets(
            yt_token=self.__read_token('~/.keys/robot_cloud_ai_yt_token'),
            mds_static_access_key=self.__read_token('~/.keys/cloud_nirvana_key_identifier'),
            mds_static_secret_key=self.__read_token('~/.keys/cloud_nirvana_secret_key'),
            mds_service_id='961',
            registry_key_path='~/.keys/cloud-nirvana-docker-key.json'
        )

        return RunJobRequest(job_json, layers, secrets)

    def test_run_simple_job(self):
        run_job_request = self.get_run_job_request()

        with tempfile.TemporaryDirectory() as tmp_dir:
            layer_manager = LayerManager(tmp_dir)

            job = Job(
                run_job_request.job_json,
                run_job_request.layers,
                run_job_request.secrets,
                DockerCreator.DEFAULT_DOCKER_IMAGE,
                layer_manager,
                tmp_dir,
                './nirvana_bundle'
            )
            job.run()

            while True:
                time.sleep(0.1)
                status = job.status
                if status == JobStatus.FAILED or status == JobStatus.COMPLETED:
                    break
                else:
                    assert status == JobStatus.RUNNING or \
                           status == JobStatus.CONVERTING_PORTO_LAYERS or \
                           status == JobStatus.BUILDING_IMAGE

            assert status == JobStatus.COMPLETED

    def test_job_manager(self):
        run_job_request = self.get_run_job_request()

        with tempfile.TemporaryDirectory() as tmp_dir:
            job_manager = HostJobManager(working_dir=tmp_dir, layers_dir=tmp_dir)
            run_job_status = job_manager.run_job(
                run_job_request.job_json,
                run_job_request.layers,
                run_job_request.secrets.yt_token,
                run_job_request.secrets.mds_static_access_key,
                run_job_request.secrets.mds_static_secret_key,
                run_job_request.secrets.mds_service_id,
                run_job_request.secrets.registry_key_string
            )
            assert run_job_status == JobStatus.ACCEPTED

            job_id = parse_job_uid(run_job_request.job_json)
            while True:
                time.sleep(0.1)
                status = job_manager.get_job_status(job_id).status
                if status == JobStatus.FAILED or status == JobStatus.COMPLETED:
                    break
                else:
                    assert status == JobStatus.RUNNING

            assert status == JobStatus.COMPLETED


if __name__ == '__main__':
    unittest.main()
