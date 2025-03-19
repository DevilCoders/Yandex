from typing import Tuple

from cloud.dwh.nirvana.config import DeployContext
from cloud.dwh.nirvana.vh.common.workflows import HourlyRandomMinuteCronReactiveWorkflowBase
from cloud.dwh.nirvana.vh.common.workflows import SingleYqlWithChecksReactiveWorkflowBase
from cloud.dwh.nirvana.vh.config.base import BaseDeployConfig as DeployConfig


class ReactiveWorkflow(HourlyRandomMinuteCronReactiveWorkflowBase, SingleYqlWithChecksReactiveWorkflowBase):
    def _get_workflow_tags(self) -> Tuple[str, ...]:
        return 'dm', 'crm', 'calls'

    def _get_reaction_retries(self) -> int:
        return 3


def main(config: DeployConfig) -> DeployContext:
    return ReactiveWorkflow(__name__).build_graph(deploy_config=config)
