import abc
import functools
import logging
import os
import random
from typing import Dict
from typing import List
from typing import Optional
from typing import Tuple

import vh
import yaml
from library.python import resource

from cloud.dwh.nirvana import reactor
from cloud.dwh.nirvana.config import DeployContext
from cloud.dwh.nirvana.config import NirvanaDeployContext
from cloud.dwh.nirvana.vh.config.base import BaseDeployConfig as DeployConfig

LOG = logging.getLogger(__name__)

VH_MODULE = __package__.rsplit(sep='.', maxsplit=2)[0]
VH_MODULE_PATH = VH_MODULE.replace('.', '/')


class WorkflowBase(abc.ABC):
    """
    Generate workflow graph without reaction.
    Abstract class.
    You need to implement:
    * `_get_workflow_tags` - additional workflow tags
    * `_fill_graph` - graph body

    And you have to add file with parameters. Default path to `./resources/parameters.yaml`
    File format:
    {enviroment name (prod/preprod)}:
      workflow_guid: "..."  # Nirvana workflow guid. It may be empty for new workflow, but then get it from the log `..Nirvana workflow guid: ..`
          ...
    """

    def __init__(self, full_module_name: str, *, vh_workflows_module_path: str = VH_MODULE_PATH):
        self._full_module_name = full_module_name

        self._full_module_path = self._full_module_name.replace('.', '/')
        self._relative_module_path = self._full_module_path.removeprefix(vh_workflows_module_path).lstrip('/').removeprefix('workflows/')

        self._dwh_layer = self._relative_module_path.split(sep='/', maxsplit=1)[0]

        LOG.debug('Layer %s. Workflow from %s', self._dwh_layer, self._relative_module_path)

    @property
    def _resource_folder(self):
        """
        Folder with resource files.
        Default to ./resources
        """
        return os.path.join(self._relative_module_path, 'resources')

    @property
    def _parameters_path(self):
        """
        Path to parameters.yaml file.
        Default to ./resources/parameters.yaml
        """
        return os.path.join(self._resource_folder, 'parameters.yaml')

    @functools.cache
    def _get_parameters(self) -> dict:
        """
        Load parameters.yaml from file (cached)
        """
        LOG.debug('Loading parameters from %s', self._parameters_path)

        parameters = yaml.safe_load(resource.find(self._parameters_path))
        LOG.debug('Workflow parameters is %s', self._parameters_path)

        return parameters

    def _get_environment_parameters(self, environment: str) -> dict:
        """
        Get parameters from parameters.yaml for specific environment
        """
        return self._get_parameters()[environment]

    def _get_workflow_name(self, environment: str) -> str:
        """
        Workflow name.
        Default to `[{environment}] [{dwh layer}] {short module path}
        """
        return f'[{environment}] [{self._dwh_layer}] {self._relative_module_path.replace("/", ".").removeprefix(self._dwh_layer + ".")}'

    def _get_base_workflow_tags(self) -> Tuple[str, ...]:
        """
        Base workflow tags.
        Default to `({dwh layer},)`
        """
        return self._dwh_layer,

    @abc.abstractmethod
    def _get_workflow_tags(self) -> Tuple[str, ...]:
        """
        Tags that are additional to base workflow tags.
        """
        return ()

    def build_graph(self, deploy_config: DeployConfig) -> DeployContext:
        """
        Entry point for building the graph.
        """
        with vh.Graph() as graph:
            self._fill_graph(graph=graph, deploy_config=deploy_config)
            deploy_context = self._fill_deploy_context(graph=graph, deploy_config=deploy_config)

            LOG.debug('Workflow deploy context: %s', deploy_context)

            return deploy_context

    @abc.abstractmethod
    def _fill_graph(self, graph: vh.Graph, deploy_config: DeployConfig):
        """
        Fill graph with operations.
        """
        raise NotImplementedError()

    def _fill_deploy_context(self, graph: vh.Graph, deploy_config: DeployConfig) -> DeployContext:
        """
        Fill deploy parameters
        """
        run_config = NirvanaDeployContext(
            workflow_guid=self._get_environment_parameters(environment=deploy_config.environment).get('workflow_guid'),
            label=self._get_workflow_name(environment=deploy_config.environment),
            workflow_tags=(self._get_base_workflow_tags() or ()) + (self._get_workflow_tags() or ()),
        )

        return DeployContext(
            run_config=run_config,
            graph=graph,
        )


class ReactiveWorkflowBase(WorkflowBase, abc.ABC):
    """
    Generates workflow with reaction
    """

    REACTION_RETRIES: Optional[int] = 5
    CRON_SCHEDULE: Optional[str] = None

    def _get_reaction_name(self) -> str:
        """
        Reaction name.
        Default to `{module name}`
        """
        return self._relative_module_path.rsplit(sep='/', maxsplit=1)[1]

    def _get_artifact(self, reaction_folder: str) -> Optional[reactor.Artifact]:
        """
        Get reaction artifact name.
        Default to `{module name}_ready`
        """
        artifact_name = f'{self._get_reaction_name()}_ready'
        artifact_path = os.path.join(reaction_folder, artifact_name)

        return reactor.Artifact(path=artifact_path)

    @abc.abstractmethod
    def _get_artifact_success_result(self, deploy_config: DeployConfig) -> Optional[str]:
        """
        Get reaction artifact success result.
        """
        pass

    @abc.abstractmethod
    def _get_artifact_failure_result(self, deploy_config: DeployConfig) -> Optional[str]:
        """
        Get reaction artifact failure result.
        """
        pass

    def _get_reaction_retries(self) -> int:
        """
        Get number of reaction retries.
        """
        return self.REACTION_RETRIES

    def _get_input_triggers(self, deploy_config: DeployConfig) -> Optional[List[reactor.InputTrigger]]:
        """
        Get reaction input triggers
        """
        return None

    def _get_input_triggers_type(self) -> Optional[reactor.InputTriggerType]:
        """
        Get reaction input triggers type
        """
        return None

    def _get_reaction_schedule(self) -> Optional[str]:
        """
        Get reaction CRON-like schedule
        """
        return self.CRON_SCHEDULE

    def _get_reaction_sequential_queue(self, deploy_config: DeployConfig) -> Optional[str]:
        """
        Get reaction sequential queue - queue that provides a sequential start for each reaction
        (only one running instance is used for each reaction)
        """
        return os.path.join(deploy_config.reactor_path_prefix, self._dwh_layer, 'sequential_queue')

    def _fill_deploy_context(self, graph: vh.Graph, deploy_config: DeployConfig) -> DeployContext:
        """
        Fill deploy parameters
        """
        deploy_context = super()._fill_deploy_context(graph=graph, deploy_config=deploy_config)

        reaction_folder = os.path.join(deploy_config.reactor_path_prefix, self._relative_module_path)
        reaction_path = os.path.join(reaction_folder, self._get_reaction_name())

        artifact = self._get_artifact(reaction_folder=reaction_folder)
        if artifact:
            deploy_context.artifact = artifact
            deploy_context.artifact_success_result = self._get_artifact_success_result(deploy_config=deploy_config)
            deploy_context.artifact_failure_result = self._get_artifact_failure_result(deploy_config=deploy_config)

        deploy_context.reaction_path = reaction_path
        deploy_context.retries = self._get_reaction_retries()

        deploy_context.schedule = self._get_reaction_schedule()

        deploy_context.input_triggers = self._get_input_triggers(deploy_config=deploy_config)
        deploy_context.input_triggers_type = self._get_input_triggers_type()

        deploy_context.sequential_queue = self._get_reaction_sequential_queue(deploy_config=deploy_config)

        return deploy_context


class HourlyRandomMinuteCronReactiveWorkflowBase(ReactiveWorkflowBase, abc.ABC):
    """
    Run workflow via reaction every hour and random minute
    """

    CRON_SCHEDULE = '0 {minute} * * * ? *'

    def _get_reaction_schedule(self) -> Optional[str]:
        """
        Get reaction CRON-like schedule
        """
        return self.CRON_SCHEDULE.format(minute=random.randint(0, 59))


class InputTriggerBlockReactiveWorkflowBase(ReactiveWorkflowBase, abc.ABC):
    """
    Run workflow via reaction if artifact was triggered.
    Artifact output used as operation block parameter.
    """

    @abc.abstractmethod
    def _get_input_trigger_block_param_name(self) -> str:
        """
        Artifact's result will be placed in this parameter name of the workflow operation
        """
        pass

    def _get_block_codes_by_artifact(self, deploy_config: DeployConfig) -> Dict[reactor.Artifact, str]:
        """
        Names of block codes by artifact. Block code used to refer to operation
        """
        artifacts = self._get_environment_parameters(environment=deploy_config.environment)['input_triggers']
        artifacts = artifacts if isinstance(artifacts, list) else [artifacts]
        return {reactor.Artifact(path=a): f'artifact_{i}' for i, a in enumerate(artifacts)}

    def _get_input_triggers(self, deploy_config: DeployConfig) -> Optional[List[reactor.InputTrigger]]:
        """
        Create input triggers for reaction
        """
        block_codes_by_artifact = self._get_block_codes_by_artifact(deploy_config=deploy_config)
        input_triggers = []

        for artifact, block_code in block_codes_by_artifact.items():
            input_triggers.append(reactor.InputTrigger(block_code=block_code, param_name=self._get_input_trigger_block_param_name(), artifact=artifact))

        return input_triggers

    @abc.abstractmethod
    def _get_input_triggers_type(self) -> Optional[reactor.InputTriggerType]:
        """
        Get input triggers type (any/all)
        """
        pass
