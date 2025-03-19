from typing import Tuple

from cloud.dwh.nirvana.config import DeployContext
from cloud.dwh.nirvana.vh.common.workflows import HourlyRandomMinuteCronReactiveWorkflowBase
from cloud.dwh.nirvana.vh.common.workflows import SingleYqlReactiveWorkflowBase
from cloud.dwh.nirvana.vh.config.base import BaseDeployConfig as DeployConfig


class ReactiveWorkflow(HourlyRandomMinuteCronReactiveWorkflowBase, SingleYqlReactiveWorkflowBase):
    def _get_workflow_tags(self) -> Tuple[str, ...]:
        return 'crm', 'crm_product_categories'


def main(config: DeployConfig) -> DeployContext:
    return ReactiveWorkflow(__name__).build_graph(deploy_config=config)
