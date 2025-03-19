import abc
import dataclasses
from typing import List
from typing import Tuple

import reactor_client

from cloud.dwh.nirvana import reactor


@dataclasses.dataclass(frozen=True)
class BaseDeployConfig(abc.ABC):
    vh_module: str = None

    environment: str = None
    module: str = None
    deploy_mode: str = None
    nirvana_oauth_token: str = None

    debug: bool = False

    reactor_prefix: str = None
    reactor_owner: str = None

    nirvana_workflow_group_ns_id: int = None
    nirvana_quota: str = None
    nirvana_tags: Tuple[str] = ()
    use_threaded_api: bool = True

    @property
    def vh_workflows_module(self):
        return f'{self.vh_module}.workflows'

    @property
    def vh_workflows_module_path(self):
        return self.vh_workflows_module.replace('.', '/')

    @property
    def reactor_path_prefix(self):
        return f'{self.reactor_prefix}/{self.environment}/'


@dataclasses.dataclass
class NirvanaRunConfig:
    workflow_ns_id: int
    threaded_api: bool
    oauth_token: str
    workflow_tags: Tuple[str]
    quota: str

    start: bool = dataclasses.field(default=False)
    global_options: dict = dataclasses.field(default_factory=dict)


@dataclasses.dataclass
class NirvanaDeployContext:
    label: str
    workflow_guid: str = None
    workflow_tags: Tuple[str, ...] = dataclasses.field(default_factory=tuple)


@dataclasses.dataclass
class DeployContext:
    run_config: NirvanaDeployContext
    graph: object
    reaction_path: str = dataclasses.field(default=None)

    sequential_queue: str = dataclasses.field(default=None)

    artifact: reactor.Artifact = dataclasses.field(default=None)
    artifact_success_result: str = dataclasses.field(default=None)
    artifact_failure_result: str = dataclasses.field(default=None)

    cleanup_ttl: int = dataclasses.field(default=3)
    nirvana_instances_ttl: int = dataclasses.field(default=14)

    retries: int = dataclasses.field(default=0)
    retry_time_param: int = dataclasses.field(default=10)
    retry_policy: reactor_client.r_objs.RetryPolicyDescriptor = dataclasses.field(
        default=reactor_client.r_objs.RetryPolicyDescriptor.UNIFORM
    )

    schedule: str = dataclasses.field(default=None)
    # https://wiki.yandex-team.ru/nirvana/reactor/reaction/trigger/cron/#politikaobrabotkiosechek
    schedule_misfire_policy: reactor_client.r_objs.MisfirePolicy = dataclasses.field(
        default=reactor_client.r_objs.MisfirePolicy.FIRE_ONE
    )

    input_triggers: List[reactor.InputTrigger] = dataclasses.field(default=None)
    input_triggers_type: reactor.InputTriggerType = dataclasses.field(default=reactor.InputTriggerType.ALL)

    @property
    def success_expression(self):
        if self.artifact_success_result is not None:
            return f"a'{self.artifact.path}'.instantiate(\"{self.artifact_success_result}\");"
        return ''

    @property
    def failure_expression(self):
        if self.artifact_failure_result is not None:
            return f"a'{self.artifact.path}'.instantiate(\"{self.artifact_failure_result}\");"
        return ''
