from cloud.blockstore.sandbox.utils import constants

from sandbox import sdk2
from sandbox.common.types import resource as ctr
from sandbox.projects.common.teamcity import TeamcityArtifactsContext

import logging
import os

LOGGER = logging.getLogger('artifact_helper')


class ArtifactHelper:
    def __init__(self, task, artifacts_directory):
        self._task = task
        self._artifacts_directory = self._task.path(artifacts_directory)

    def _publish_artifact(self, artifact_type, teamcity_artifact_context):
        LOGGER.debug('Publishing artifact with type %s', artifact_type)

        if artifact_type == constants.DEFAULT_TEAMCITY_ARTIFACTS_TYPE:
            teamcity_artifact_context.logger.info(
                "##teamcity[publishArtifacts '{} => sandbox_artifacts.zip']".format(
                    artifact_type
                )
            )
        elif artifact_type == constants.DEFAULT_TEAMCITY_SERVICE_MESSAGES_LOG_TYPE:
            resource_path = self._artifacts_directory / artifact_type
            if resource_path.is_file():
                with open(str(resource_path)) as f:
                    for line in f:
                        teamcity_artifact_context.logger.info(line.rstrip())
        else:
            sdk2.ResourceData(sdk2.Resource[artifact_type](
                self._task,
                'Resource of type {}'.format(artifact_type),
                self._artifacts_directory / artifact_type,
            )).ready()

    def publish_artifacts(self):
        if not self._artifacts_directory.exists():
            return dict()

        LOGGER.debug('Publishing all artifacts')

        with TeamcityArtifactsContext(
            self._artifacts_directory,
            tc_service_messages_ttl=constants.DEFAULT_TEAMCITY_SERVICE_MESSAGES_LOG_TTL,
            tc_artifacts_ttl=constants.DEFAULT_TEAMCITY_ARTIFACTS_TTL,
        ) as tac:
            for artifacts_type in os.listdir(str(self._artifacts_directory)):
                LOGGER.debug(
                    'Parent dir: %s, subdir: %s',
                    self._artifacts_directory,
                    artifacts_type
                )
                self._publish_artifact(artifacts_type, tac)

    def copy_artifacts(
        self,
        task_id,
        source_type,
        source_path,
        destination_type,
        destination_path
    ):
        src_resource = find_resource(
            source_type,
            task_id,
            ctr.State.READY,
        )
        if src_resource is None:
            return

        LOGGER.debug('Copy artifacts to destination')

        source = sdk2.ResourceData(src_resource).path
        if len(source_path) != 0:
            source = sdk2.ResourceData(src_resource).path / source_path
        if not source.exists():
            return

        LOGGER.debug('Copy artifact from: %s', source)

        destination = self._artifacts_directory / destination_type
        if len(destination_path) != 0:
            destination = destination / destination_path
        if not destination.parent.exists():
            destination.parent.mkdir(parents=True)

        LOGGER.debug('Paste artifact to: %s', destination)

        with open(str(source)) as src, open(str(destination), 'a+') as dst:
            dst.write(src.read())


def find_resource(resource_type, task_id, state):
    LOGGER.debug('try_to_find_resource: resource_type={}, task_id={}, state={}'.format(
        resource_type,
        task_id,
        state,
    ))
    resource = sdk2.Resource.find(
        type=resource_type,
        task_id=task_id,
        state=state,
    ).first()

    if not resource:
        LOGGER.error(
            'Cannot find resource of type %s in task %s with state %s',
            resource_type,
            task_id,
            state,
        )
        return None

    return resource
