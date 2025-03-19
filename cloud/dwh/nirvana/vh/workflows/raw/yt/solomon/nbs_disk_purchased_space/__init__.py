from typing import Tuple

from cloud.dwh.nirvana.config import DeployContext
from cloud.dwh.nirvana.vh.common.workflows import HourlyRandomMinuteCronReactiveWorkflowBase
from cloud.dwh.nirvana.vh.common.workflows import SolomonToYtReactiveWorkflowBase
from cloud.dwh.nirvana.vh.config.base import BaseDeployConfig as DeployConfig


class ReactiveWorkflow(HourlyRandomMinuteCronReactiveWorkflowBase, SolomonToYtReactiveWorkflowBase):
    def _get_workflow_tags(self) -> Tuple[str, ...]:
        return 'solomon', 'nbs'


def main(config: DeployConfig) -> DeployContext:
    return ReactiveWorkflow(__name__).build_graph(deploy_config=config)
