import logging
import time

import dataclasses
import reactor_client as reactor
from enum import Enum

from reactor_client.reaction_builders import NirvanaReactionBuilder

LOG = logging.getLogger(__name__)

REACTOR_BASE_URL = "https://reactor.yandex-team.ru"


class InputTriggerType(Enum):
    ALL = 'all_inputs'
    ANY = 'any_input'


@dataclasses.dataclass(frozen=True)
class Artifact:
    path: str

    type: str = dataclasses.field(default=reactor.r_objs.ArtifactTypes.PRIMITIVE_STRING.name)
    description: str = dataclasses.field(default='Artifact')
    ttl_days: int = dataclasses.field(default=1)


@dataclasses.dataclass(frozen=True)
class Queue:
    path: str

    max_running_instances: int = -1
    priority_function: reactor.r_objs.QueuePriorityFunction = dataclasses.field(default=reactor.r_objs.QueuePriorityFunction.TIME_ELDEST_FIRST)
    max_queued_instances: int = 1000
    max_running_instances_per_reaction: int = 1
    max_queued_instances_per_reaction: int = 1


@dataclasses.dataclass(frozen=True)
class InputTrigger:
    block_code: str
    param_name: str
    artifact: Artifact


class ReactorWrapper:
    DEACTIVATION_CHECK_PERIOD = 1  # 1 second
    NOTIFICATION_TELEGRAM_PROD = 'telegram:-1001340496133'
    NOTIFICATION_TELEGRAM_PREPROD = 'telegram:-1001563392171'

    def __init__(self, token, base_url=REACTOR_BASE_URL, **kwargs):
        self.client = reactor.ReactorAPIClientV1(
            base_url=base_url,
            token=token,
            **kwargs
        )

    def replace_reaction(self, builder: NirvanaReactionBuilder):
        namespace_id = builder.operation_descriptor.namespace_desc.namespace_identifier
        existing_operation_id = reactor.r_objs.OperationIdentifier(namespace_identifier=namespace_id)

        if self.client.reaction.check_exists(existing_operation_id):
            LOG.info('Deactivate existing reaction')
            response = self.client.reaction.update([
                reactor.r_objs.ReactionStatusUpdate(existing_operation_id, reactor.r_objs.StatusUpdate.DEACTIVATE),
            ])
            LOG.debug(response)

            status = ''
            while status != reactor.r_objs.OperationStatus.INACTIVE:
                response = self.client.reaction.get(existing_operation_id)
                status = response.status
                LOG.info('Waiting for deactivation...')
                LOG.debug(response)
                time.sleep(self.DEACTIVATION_CHECK_PERIOD)

            LOG.info('Deleting existing reaction')
            response = self.client.namespace.delete([namespace_id], delete_if_exist=True)
            LOG.debug(response)

        LOG.info('Creating new reaction')
        reaction_ref = self.client.reaction.create(builder.operation_descriptor, create_if_not_exist=True)
        LOG.debug(reaction_ref)

        LOG.info('Created reaction %s/reaction?id=%s&mode=overview', REACTOR_BASE_URL, reaction_ref.namespace_id)
        operation_id = reactor.r_objs.OperationIdentifier(reaction_ref.reaction_id)

        LOG.info('Activating new reaction')
        response = self.client.reaction.update([
            reactor.r_objs.ReactionStatusUpdate(operation_id, reactor.r_objs.StatusUpdate.ACTIVATE),
        ])
        LOG.debug(response)

    def add_notifications(self, builder: NirvanaReactionBuilder, env: str):
        namespace_id = builder.operation_descriptor.namespace_desc.namespace_identifier

        channel = self.NOTIFICATION_TELEGRAM_PROD
        if 'preprod' in env.lower():
            channel = self.NOTIFICATION_TELEGRAM_PREPROD

        notify = reactor.r_objs.NotificationDescriptor(
            namespace_identifier=namespace_id,
            event_type=reactor.r_objs.NotificationEventType.REACTION_FAILED,
            transport=reactor.r_objs.NotificationTransportType.TELEGRAM,
            recipient=channel
        )

        self.client.namespace_notification.change(notification_descriptor_list=[notify])

    def create_queue(self, queue: Queue, create_if_not_exist: bool = True) -> Queue:
        LOG.debug('Creating queue %s', queue)

        response = self.client.queue.create(
            queue_descriptor=reactor.r_objs.QueueDescriptor(
                namespace_descriptor=reactor.r_objs.NamespaceDescriptor(
                    namespace_identifier=reactor.r_objs.NamespaceIdentifier(namespace_path=queue.path),
                ),
                configuration=reactor.r_objs.QueueConfiguration(
                    parallelism=queue.max_running_instances,
                    priority_function=queue.priority_function,
                    max_queued_instances=reactor.r_objs.QueueMaxQueuedInstances(queue.max_queued_instances),
                    max_running_instances_per_reaction=reactor.r_objs.QueueMaxRunningInstancesPerReaction(queue.max_running_instances_per_reaction),
                    max_queued_instances_per_reaction=reactor.r_objs.QueueMaxQueuedInstancesPerReaction(queue.max_queued_instances_per_reaction),
                ),
            ),
            create_if_not_exist=create_if_not_exist,
        )
        LOG.debug(response)

        LOG.debug('Queue is created')

        return queue

    def add_to_queue(self, builder: NirvanaReactionBuilder, queue: Queue):
        namespace_id = builder.operation_descriptor.namespace_desc.namespace_identifier

        LOG.debug('Add reaction %s to queue %s', namespace_id, queue)

        operation_id = reactor.r_objs.OperationIdentifier(namespace_identifier=namespace_id)

        response = self.client.queue.update(
            queue_identifier=reactor.r_objs.QueueIdentifier(
                namespace_identifier=reactor.r_objs.NamespaceIdentifier(namespace_path=queue.path),
            ),
            add_reactions=[operation_id],
        )
        LOG.debug('Add to queue response: %s', response)

    def create_artifact(self, artifact: Artifact, force: bool = False) -> Artifact:
        LOG.debug('Creating artifact %s', artifact)

        if self.is_artifact_exists(artifact.path):
            existing_artifact = self.get_artifact(artifact.path)

            if artifact != existing_artifact or force:
                self.remove_artifact(existing_artifact.path)

        response = self.client.artifact.create(
            artifact_type_identifier=reactor.r_objs.ArtifactTypeIdentifier(artifact_type_key=artifact.type),
            artifact_identifier=reactor.r_objs.ArtifactIdentifier(
                namespace_identifier=reactor.r_objs.NamespaceIdentifier(namespace_path=artifact.path),
            ),
            description=artifact.description,
            permissions=reactor.r_objs.NamespacePermissions(roles={}),
            create_parent_namespaces=True,
            create_if_not_exist=True,
            cleanup_strategy=reactor.r_objs.CleanupStrategyDescriptor(
                cleanup_strategies=[
                    reactor.r_objs.CleanupStrategy(
                        ttl_cleanup_strategy=reactor.r_objs.TtlCleanupStrategy(ttl_days=artifact.ttl_days),
                    ),
                ],
            ),
        )
        LOG.debug(response)

        artifact = self.get_artifact(artifact.path)
        LOG.debug('Artifact %s is created', artifact)

        return artifact

    def get_artifact(self, path: str) -> Artifact:
        LOG.debug('Getting artifact %s', path)

        artifact = self.client.artifact.get(
            namespace_identifier=reactor.r_objs.NamespaceIdentifier(namespace_path=path),
        )
        LOG.debug('Artifact getter response: %s', artifact)

        artifact_type = self.client.artifact_type.get(artifact_type_id=artifact.artifact_type_id)
        LOG.debug('ArtifactType getter response: %s', artifact_type)

        return Artifact(path=path, type=artifact_type.key)

    def is_artifact_exists(self, path: str) -> bool:
        LOG.debug('Checking artifact %s existence', path)

        result = self.client.artifact.check_exists(
            namespace_identifier=reactor.r_objs.NamespaceIdentifier(namespace_path=path),
        )

        LOG.debug('Artifact existence response: %s', result)

        return result

    def remove_artifact(self, path: str):
        LOG.debug('Removing artifact %s', path)
        response = self.client.namespace.delete([reactor.r_objs.NamespaceIdentifier(namespace_path=path)])
        LOG.debug('Artifact removing response: %s', response)
