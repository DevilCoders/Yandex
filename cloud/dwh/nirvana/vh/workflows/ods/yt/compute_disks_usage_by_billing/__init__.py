from typing import Tuple

from cloud.dwh.nirvana.config import DeployContext
from cloud.dwh.nirvana.vh.common.workflows import HourlyRandomMinuteCronReactiveWorkflowBase
from cloud.dwh.nirvana.vh.common.workflows import SingleYqlReactiveWorkflowBase
from cloud.dwh.nirvana.vh.config.base import BaseDeployConfig as DeployConfig


class ReactiveWorkflow(HourlyRandomMinuteCronReactiveWorkflowBase, SingleYqlReactiveWorkflowBase):
    def _get_workflow_tags(self) -> Tuple[str, ...]:
        return 'compute', 'disk_usage'

    def _get_artifact_success_result(self, deploy_config: DeployConfig) -> str:
        return self._get_environment_parameters(environment=deploy_config.environment)['query_parameters']['destination_folder_path']


def main(config: DeployConfig) -> DeployContext:
    return ReactiveWorkflow(__name__).build_graph(deploy_config=config)
