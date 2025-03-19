from typing import Optional, List
from typing import Tuple

from cloud.dwh.nirvana import reactor
from cloud.dwh.nirvana.config import DeployContext
from cloud.dwh.nirvana.vh.common.workflows import InputTriggerMultiMrTableSingleYqlReactiveWorkflowBase
from cloud.dwh.nirvana.vh.config.base import BaseDeployConfig as DeployConfig


class ReactiveWorkflow(InputTriggerMultiMrTableSingleYqlReactiveWorkflowBase):
    def _get_input_triggers_type(self) -> Optional[reactor.InputTriggerType]:
        return reactor.InputTriggerType.ANY

    def _get_workflow_tags(self) -> Tuple[str, ...]:
        return 'dm', 'marketing', 'attribution'

    def _get_yql_query_utils_files(self) -> List[str]:
        return [
            'cdm/yt/marketing/dm_marketing_attribution/resources/utils/attribution_functions.sql',
            'cdm/yt/marketing/dm_marketing_attribution/resources/udf/attribution_functions.py'
        ]


def main(config: DeployConfig) -> DeployContext:
    return ReactiveWorkflow(__name__).build_graph(deploy_config=config)
