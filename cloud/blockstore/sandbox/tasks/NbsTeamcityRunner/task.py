from cloud.blockstore.sandbox.utils import (
    artifact_helper,
    config_helper,
    constants,
)
from cloud.blockstore.sandbox.utils.arc_helper import ArcHelper
from cloud.blockstore.sandbox.utils.artifact_helper import ArtifactHelper

from sandbox import sdk2
from sandbox.common.errors import TaskFailure
from sandbox.common.types import resource as ctr
from sandbox.common.types import task as ctt

import json
import logging
import os

LOGGER = logging.getLogger('nbs_teamcity_runner')


class Task():
    def __init__(self, task):
        self._task = task

    def mount_point(self):
        return self._task.Context.mount_point

    def root_path(self):
        return os.path.join(
            self.mount_point(),
            self._task.Context.root_path,
        )

    def config_path(self):
        return os.path.join(
            self.mount_point(),
            self._task.Context.config_path,
        )

    def config(self):
        return config_helper.read_runner_config(
            self.root_path(),
            self.config_path(),
        )

    def task_config_path(self, task_name):
        return os.path.join(
            self.mount_point(),
            self._task.Context.task_config_path[task_name],
        )

    def task_config(self, task_name):
        return config_helper.read_task_config(
            self.root_path(),
            self.task_config_path(task_name),
        )

    def task_artifacts(self, task_name):
        for task in self.config().task_configs:
            if task.name == task_name:
                return task.artifacts

        raise TaskFailure('Cannot find artifact config for task {}'.format(
            task_name,
        ))

    def _process_config(self):
        LOGGER.debug('_process_config')

        self._task.Context.config_path = self._task.Parameters.config_path
        self._task.Context.root_path = self._task.Parameters.root_path
        self._task.Context.config = json.dumps(
            self.config().__dict__,
            default=lambda obj: obj.__dict__
        )

    def _setup_context(self):
        LOGGER.debug('_setup_context')

        self._task.Context.launch_step = 0
        self._task.Context.failed_task_names = []
        self._task.Context.succeed_task_names = []
        self._task.Context.running_task_names = []
        self._task.Context.not_running_task_names = []
        self._task.Context.running_task_ids = []

        self._task.Context.task_config_path = dict()
        self._task.Context.depends_on = dict()
        self._task.Context.task_config = dict()
        self._task.Context.task_name_to_task_id = dict()

        for task in self.config().task_configs:
            self._task.Context.not_running_task_names.append(task.name)
            self._task.Context.task_config_path[task.name] = task.config_path
            self._task.Context.depends_on[task.name] = [dep.name for dep in task.depends_on]
            self._task.Context.task_config = json.dumps(
                self.task_config(task.name).__dict__,
                default=lambda obj: obj.__dict__
            )

    def _prelaunch_preparation(self):
        LOGGER.debug('_prelaunch_preparation')

        self._process_config()
        self._setup_context()

    def _remove_from_dependencies(self, task_name_to_remove):
        LOGGER.debug('_remove_from_dependencies')

        for task_name in self._task.Context.not_running_task_names:
            if task_name_to_remove in self._task.Context.depends_on[task_name]:
                self._task.Context.depends_on[task_name].remove(
                    task_name_to_remove,
                )

    def _process_running_tasks(self):
        LOGGER.debug('_process_running_tasks')

        for task_name in self._task.Context.running_task_names[:]:
            task_id = self._task.Context.task_name_to_task_id[task_name]
            if sdk2.Task[task_id].status in list(
                ctt.Status.Group.FINISH | ctt.Status.Group.BREAK
            ):
                self._remove_from_dependencies(task_name)
                self._task.Context.running_task_names.remove(task_name)
                self._task.Context.running_task_ids.remove(task_id)
                if sdk2.Task[task_id].status not in ctt.Status.Group.SUCCEED:
                    self._task.Context.failed_task_names.append(task_name)
                else:
                    self._task.Context.succeed_task_names.append(task_name)
                self._task.Context.save()

    def _create_dependent_artifacts(self, task_name):
        LOGGER.debug('_copy_task_artifacts')

        resource_map = dict()
        for task in self.config().task_configs:
            if task.name != task_name:
                continue
            for dependent_task in task.depends_on:
                for artifact in dependent_task.artifacts:
                    resource = artifact_helper.find_resource(
                        artifact.type,
                        self._task.Context.task_name_to_task_id[dependent_task.name],
                        ctr.State.READY,
                    )
                    if not resource:
                        raise TaskFailure(
                            'Task {}:{} doesn\'t contain resource with type {}'.format(
                                dependent_task.name,
                                self._task.Context.task_name_to_task_id[dependent_task.name],
                                artifact.type,
                            ))
                    resource_map[artifact.parameter_name] = resource.id

        return resource_map

    def _prepare_task_parameters(self, task_name):
        LOGGER.debug('_prepare_task_parameters')

        artifact_map = self._create_dependent_artifacts(task_name)
        LOGGER.debug('Dependent artifacts for task {}: {}'.format(
            task_name,
            artifact_map,
        ))

        params = self.task_config(task_name).parameters.__dict__
        params.update(artifact_map)

        LOGGER.debug('{} parameters: {}'.format(task_name, params))
        return params

    def _prepare_task_requirements(self, task_name):
        LOGGER.debug('_prepare_task_requirements')

        requirements = self.task_config(task_name).requirements.__dict__

        LOGGER.debug('{} requirements: {}'.format(task_name, requirements))
        return requirements

    def _enqueue_task(self, task_name):
        LOGGER.debug('_enqueue_task')

        sub_task = sdk2.Task[self.task_config(task_name).task_type](
            self._task,
            __requirements__=self._prepare_task_requirements(task_name),
            **self._prepare_task_parameters(task_name)
        )
        sub_task.save()
        sub_task.enqueue()

        return sub_task

    def _run_tasks(self):
        LOGGER.debug('_run_tasks')

        for task_name in self._task.Context.not_running_task_names[:]:
            if len(self._task.Context.depends_on[task_name]) == 0:
                sub_task = self._enqueue_task(task_name)
                self._task.Context.running_task_ids.append(sub_task.id)
                self._task.Context.running_task_names.append(task_name)
                self._task.Context.not_running_task_names.remove(task_name)
                self._task.Context.task_name_to_task_id[task_name] = sub_task.id
                self._task.Context.save()

    def _prepare_failed_message(self):
        LOGGER.debug('_prepare_failed_message')

        return ''.join(
            [
                '<p>FAILED TASK ({name}): <a href="https://sandbox.yandex-team.ru/task/{id}/view">{id}</a>'.format(
                    name=task_name,
                    id=self._task.Context.task_name_to_task_id[task_name],
                ) for task_name in self._task.Context.failed_task_names
            ]
        )

    def _finalize(self):
        LOGGER.debug('_finalize')

        artifact_manager = ArtifactHelper(
            self._task,
            constants.DEFAULT_ARTIFACTS_DIRECTORY,
        )

        for task in self.config().task_configs:
            for artifact in self.task_artifacts(task.name):
                artifact_manager.copy_artifacts(
                    self._task.Context.task_name_to_task_id[task.name],
                    artifact.source_type,
                    artifact.source_path,
                    artifact.destination_type,
                    artifact.destination_path,
                )

        artifact_manager.publish_artifacts()

        if len(self._task.Context.failed_task_names) > 0:
            self._task.set_info(
                info=self._prepare_failed_message(),
                do_escape=False,
            )
            raise TaskFailure('Some child tasks failed')

    def _stop_task(self, task_id):
        LOGGER.debug('_stop_task')

        task = sdk2.Task[task_id]
        try:
            task.stop()
        except Exception as e:
            LOGGER.exception(e)

        LOGGER.debug('Stopped task {}'.format(task_id))

    def _stop_tasks(self):
        LOGGER.debug('_stop_tasks')

        for task_name in self._task.Context.running_task_names:
            task_id = self._task.Context.task_name_to_task_id[task_name]
            self._stop_task(task_id)
            self._task.Context.failed_task_names.append(task_name)

    def on_prepare(self):
        LOGGER.debug('on_prepare')

        self._arc = ArcHelper(self._task.Parameters.arc_token)
        self._arc.mount_repository()
        self._arc.checkout(
            self._task.Parameters.branch,
            self._task.Parameters.commit
        )
        self._task.Context.mount_point = self._arc.mount_point

        LOGGER.debug('{}'.format(self._arc.get_status()))

    def on_execute(self):
        LOGGER.debug('on_execute')

        with self._task.memoize_stage[
            'prelaunch_preparation'
        ](commit_on_entrance=False):
            self._prelaunch_preparation()
            self._task.Context.save()

        if len(self._task.Context.failed_task_names) + len(self._task.Context.succeed_task_names) < len(self.config().task_configs):
            self._task.Context.launch_step += 1
            self._process_running_tasks()
            self._task.Context.save()

            with self._task.memoize_stage[
                'run_tasks_{}_step'.format(self._task.Context.launch_step)
            ](commit_on_entrance=False):
                self._run_tasks()
                self._task.Context.save()
            with self._task.memoize_stage[
                'wait_tasks_{}_step'.format(self._task.Context.launch_step)
            ]:
                raise sdk2.WaitTask(
                    self._task.Context.running_task_ids,
                    ctt.Status.Group.FINISH | ctt.Status.Group.BREAK,
                    wait_all=False)

        self._finalize()

    def on_before_timeout(self, seconds):
        LOGGER.debug('on_before_timeout')

        self._stop_tasks()
        self._finalize()

    def on_terminate(self):
        LOGGER.debug('on_terminate')

        self._stop_tasks()
        self._finalize()
