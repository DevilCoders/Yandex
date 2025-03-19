import abc
import logging
import os
from typing import Optional

import vh

from cloud.dwh.nirvana.vh.common.operations import get_default_python_executor_op
from cloud.dwh.nirvana.vh.common.operations import get_secret_as_file_op
from cloud.dwh.nirvana.vh.common.workflows.base import ReactiveWorkflowBase
from cloud.dwh.nirvana.vh.config.base import BaseDeployConfig as DeployConfig
from cloud.dwh.nirvana.vh.common.operations import get_disable_cache_op

LOG = logging.getLogger(__name__)


class PythonReactiveWorkflowBase(ReactiveWorkflowBase, abc.ABC):

    ROOT_PATH = '../vh/workflows/'

    def _get_python_script(self) -> str:
        """
        python script from `./resources/script.py`
        """
        return os.path.join(self._resource_folder, 'script.py')

    def _get_script_parameters(self, deploy_config: DeployConfig) -> dict:
        """
        Python script parameters from `./resources/parameters.yaml:{environment}:script_parameters`
        """
        return self._get_environment_parameters(environment=deploy_config.environment).get('script_parameters', {})

    def _get_artifact_success_result(self, deploy_config: DeployConfig) -> Optional[str]:
        """
        Artifact success result is `./resources/parameters.yaml:{environment}:destination_path`
        """
        return self._get_environment_parameters(environment=deploy_config.environment).get('destination_path', '')

    def _get_artifact_failure_result(self, deploy_config: DeployConfig) -> Optional[str]:
        """
        No artifact on failure
        """
        return None

    def _fill_graph(self, graph: vh.Graph, deploy_config: DeployConfig):
        """
        Fill graph with operations.
        """

        param_dict = self._get_script_parameters(deploy_config)
        param_dict['yt_cluster'] = deploy_config.yt_cluster
        parameters = self._get_environment_parameters(environment=deploy_config.environment)
        if 'destination_path' in parameters:
            param_dict['destination_path'] = parameters['destination_path']

        yt_token_to_file = get_secret_as_file_op(deploy_config, deploy_config.yt_token)
        yt_token_to_file_result = yt_token_to_file(_name='YT Token Provider')

        staff_token_to_file = get_secret_as_file_op(deploy_config, deploy_config.staff_token)
        staff_token_to_file_result = staff_token_to_file(_name='Staff API Token Provider')

        disable_cache_op_result = get_disable_cache_op()

        py_op = get_default_python_executor_op(deploy_config, disable_cache_op_result=disable_cache_op_result)
        py_op(
            _name='Run python script',
            _inputs={
                'script': self.ROOT_PATH + self._get_python_script(),
                'infiles': [yt_token_to_file_result['secret'], staff_token_to_file_result['secret']]
            },
            _options={
                'input-params': ';'.join([f'{name}={param}' for name, param in param_dict.items()]),
            },
        )
