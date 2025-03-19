from typing import Tuple
from typing import List

from cloud.dwh.nirvana.config import DeployContext
from cloud.dwh.nirvana.vh.common.workflows import HourlyRandomMinuteCronReactiveWorkflowBase
from cloud.dwh.nirvana.vh.common.workflows import SingleYqlReactiveWorkflowBase
from cloud.dwh.nirvana.vh.config.base import BaseDeployConfig as DeployConfig


class ReactiveWorkflow(HourlyRandomMinuteCronReactiveWorkflowBase, SingleYqlReactiveWorkflowBase):
    def _get_workflow_tags(self) -> Tuple[str, ...]:
        return 'billing', 'billing_accounts_history'

    def _get_yql_query_utils_files(self) -> List[str]:
        return ['ods/yt/billing/billing_accounts/resources/utils/billing_accounts.sql']


def main(config: DeployConfig) -> DeployContext:
    return ReactiveWorkflow(__name__).build_graph(deploy_config=config)
