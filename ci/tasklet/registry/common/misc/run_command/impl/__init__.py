# coding=utf-8

import contextlib
import json
import logging
import os
import shutil
import sys

import subprocess

from sandbox.common.mds.compression.base import CompressionType
import sandbox.sdk2 as sandbox_sdk2
from sandbox.projects.common.vcs import arc as lib_arc

from tasklet.services.ci import get_ci_env
from tasklet.services.yav.proto import yav_pb2 as yav
from ci.tasklet.common.proto import service_pb2 as ci
from ci.tasklet.registry.common.misc.run_command.proto import run_command_tasklet
from google.protobuf.json_format import Parse

SECRET_FILES_PATH = 'secret_files'
RESULT_RESOURCES_PATH = 'result_resources'
RESULT_EXTERNAL_RESOURCES_PATH = 'result_external_resources'
RESULT_BADGES_PATH = 'result_badges'
ARCADIA_PATH = 'mounted_arcadia'

COMPRESSION_TYPE_RESOLVING = {
    k.lower(): v
    for k, v in CompressionType.iteritems()
}
DEFAULT_COMPRESSION_TYPE = 'tgz'


class RunCommandImpl(run_command_tasklet.RunCommandBase):
    def run(self):
        secret_dir = self.init_secret_dir()

        self.output.state.message = 'internal-error'
        self.output.state.success = False
        self.output.state.return_code = -1

        self.task = sandbox_sdk2.Task.current
        self.errors = []
        try:
            env = self.prepare_environment(secret_dir)
            placeholder = self.prepare_placeholder()
            cmd = self.input.config.cmd_line
            if placeholder:
                cmd = cmd.format(**placeholder)

            with self.mount_arc() as env_path:
                env.update(env_path)
                self.update_ya_token(env)
                self.run_command(cmd, env)
        finally:
            self.cleanup_secret_dir(secret_dir)

        self.save_resources()
        self.save_external_resources()
        self.save_output()
        self.save_badges()

        self.check_for_errors()

        self.output.state.message = 'ok'
        self.output.state.success = True
        self.output.state.return_code = 0

    def init_secret_dir(self):
        abs_dir = os.path.abspath(SECRET_FILES_PATH)
        os.makedirs(abs_dir, exist_ok=True)
        return abs_dir

    def cleanup_secret_dir(self, secret_dir):
        if secret_dir and os.path.isdir(secret_dir):
            shutil.rmtree(secret_dir)

    def _setup_uuid(self, spec):
        if not spec.uuid:
            assert self.input.context.secret_uid, 'Internal error, secret_uid is not available in context'
            spec.uuid = self.input.context.secret_uid

    def prepare_environment(self, secret_dir):
        # Временное решение для ускорения медленного монтирования arc
        # https://st.yandex-team.ru/ARC-3966
        os.environ['ARC_SKIP_SERVER_CACHE'] = 'true'

        env = os.environ.copy()

        if self.task:
            ramdrive = self.task.ramdrive
            if ramdrive:
                env['SANDBOX_TMP'] = ramdrive.path

            env['SANDBOX_TASK_ID'] = str(self.task.id)
            env['SANDBOX_TASK_OWNER'] = self.task.owner

        for var in self.input.config.environment_variables:
            assert var.key, 'key field missing or empty in {}'.format(var)
            if var.value:
                env[var.key] = var.value

        for var in self.input.config.secret_environment_variables:
            assert var.key, 'key field missing or empty in {}'.format(var)
            assert var.secret_spec, 'secret_spec field missing or empty in {}'.format(var)
            assert var.secret_spec.key, 'key field missing or empty in secret_spec for {}'.format(var)
            self._setup_uuid(var.secret_spec)
            env[var.key] = self.ctx.yav.get_secret(var.secret_spec).secret

        for var in self.input.config.secret_file_variables:
            assert var.key, 'key field missing or empty in {}'.format(var)
            assert var.secret_spec, 'secret_spec field missing or empty in {}'.format(var)
            assert var.secret_spec.key, 'key field missing or empty in secret_spec for {}'.format(var)
            self._setup_uuid(var.secret_spec)
            filename = os.path.join(secret_dir, var.key)
            with open(filename, 'w') as f:
                f.write(self.ctx.yav.get_secret(var.secret_spec).secret)
            env[var.key] = filename

        if self.input.config.result_resources or self.input.config.result_output:
            abs_dir = os.path.abspath(RESULT_RESOURCES_PATH)
            os.makedirs(abs_dir, exist_ok=True)
            env['RESULT_RESOURCES_PATH'] = abs_dir

        if self.input.config.result_external_resources_from_files:
            abs_dir = os.path.abspath(RESULT_EXTERNAL_RESOURCES_PATH)
            os.makedirs(abs_dir, exist_ok=True)
            env['RESULT_EXTERNAL_RESOURCES_PATH'] = abs_dir

        if self.input.config.result_badges:
            abs_dir = os.path.abspath(RESULT_BADGES_PATH)
            os.makedirs(abs_dir, exist_ok=True)
            env['RESULT_BADGES_PATH'] = abs_dir

        if self.task:
            author = self.task.author
            env['YA_USER'] = author
            env['LOGNAME'] = author

        return env

    @staticmethod
    def _get_proxy_resource_link(resource_id, filename=None):
        if filename:
            return "https://proxy.sandbox.yandex-team.ru/{}/{}".format(resource_id, filename)
        else:
            return "https://proxy.sandbox.yandex-team.ru/{}".format(resource_id)

    def _get_resource_link(self, resource_id, filename=None):
        if self.input.config.direct_output_links or filename:
            return self._get_proxy_resource_link(resource_id, filename)
        else:
            return "https://sandbox.yandex-team.ru/resource/{}/view".format(resource_id)

    def _find_resource(self, **constraints):
        resources = sandbox_sdk2.Resource.find(**constraints)
        assert resources, "Unable to find resources by {}".format(constraints)
        first = resources.first()
        assert first, "Unable to find first resource by {}".format(constraints)
        return first

    def _download_resource(self, **constraints):
        resource = self._find_resource(**constraints)
        data = sandbox_sdk2.ResourceData(resource)
        return {'id': resource.id, 'value': str(data.path)}

    def _get_stderr(self, stderr):
        if self.input.config.logs_config.redirect_stderr_to_stdout:
            return subprocess.STDOUT
        else:
            return stderr

    def _get_logs_configs(self):
        logs_config = self.input.config.logs_config
        yield {'id': 'stdout', 'file': 'run_command.out.log', 'text': 'Output logs', 'ci_badge': logs_config.stdout_ci_badge}
        if not logs_config.redirect_stderr_to_stdout:
            yield {'id': 'stderr', 'file': 'run_command.err.log', 'text': 'Error logs', 'ci_badge': logs_config.stderr_ci_badge}

    def _get_logs_formatter(self):
        if self.input.config.logs_config.add_timestamp:
            return logging.Formatter(fmt='[%(asctime)s] %(message)s', datefmt='%Y-%m-%d %H:%M:%S')
        return None

    def _get_task_logs_resource(self):
        if self.task:
            return self._find_resource(type='TASK_LOGS', task_id=self.task.id)
        else:
            return None

    def prepare_placeholder(self):
        placeholder = dict()
        inverted = dict()
        resource_list = []

        def add_resource(key, map):
            placeholder[key] = map['value']
            resource_list.append('<a href="{}">{}</a>'.format(self._get_proxy_resource_link(map['id']), key))

        for sandbox_resource_placeholder in self.input.config.sandbox_resource_placeholders:
            assert sandbox_resource_placeholder.key, 'key field missing or empty in {}'.format(sandbox_resource_placeholder)
            assert sandbox_resource_placeholder.resource_type, 'resource_type field missing or empty in {}'.format(sandbox_resource_placeholder)
            inverted[sandbox_resource_placeholder.resource_type] = sandbox_resource_placeholder

        preserved_types = set()
        for sb_resource in self.input.sb_resources:
            if sb_resource.type in inverted:
                assert sb_resource.type not in preserved_types, 'ambiguity of definition of {}'.format(sb_resource.type)
                preserved_types.add(sb_resource.type)
        if len(inverted) != len(preserved_types):
            missing_resources = set(inverted) - preserved_types
            assert len(missing_resources) == 0, 'not found: ' + ', '.join(missing_resources)

        for fixed_sandbox_resource in self.input.config.fixed_sandbox_resources:
            assert fixed_sandbox_resource.key, 'key field missing or empty in {}'.format(fixed_sandbox_resource)
            assert fixed_sandbox_resource.resource_id, 'resource_id field missing or empty in {}'.format(fixed_sandbox_resource)
            add_resource(fixed_sandbox_resource.key, self._download_resource(id=fixed_sandbox_resource.resource_id))

        for dynamic_sandbox_resource in self.input.config.dynamic_sandbox_resources:
            assert dynamic_sandbox_resource.key, 'key field missing or empty in {}'.format(dynamic_sandbox_resource)
            assert dynamic_sandbox_resource.type, 'type field missing or empty in {}'.format(dynamic_sandbox_resource)
            attrs = dict()
            for key in dynamic_sandbox_resource.attrs:
                attrs[key] = dynamic_sandbox_resource.attrs[key]
            state = [s for s in dynamic_sandbox_resource.state] or ['READY']
            add_resource(dynamic_sandbox_resource.key, self._download_resource(type=dynamic_sandbox_resource.type, attrs=attrs, state=state))

        for dynamic_sandbox_resource_ref in self.input.config.dynamic_sandbox_resource_refs:
            assert dynamic_sandbox_resource_ref.key, 'key field missing or empty in {}'.format(dynamic_sandbox_resource_ref)
            assert dynamic_sandbox_resource_ref.type, 'type field missing or empty in {}'.format(dynamic_sandbox_resource_ref)
            attrs = dict()
            for key in dynamic_sandbox_resource_ref.attrs:
                attrs[key] = dynamic_sandbox_resource_ref.attrs[key]
            state = [s for s in dynamic_sandbox_resource_ref.state] or ['READY']
            resource = self._find_resource(type=dynamic_sandbox_resource_ref.type, attrs=attrs, state=state)
            add_resource(dynamic_sandbox_resource_ref.key, {'id': resource.id, 'value': str(resource.id)})

        for sb_resource in self.input.sb_resources:
            if sb_resource.type in inverted:
                add_resource(inverted[sb_resource.type].key, self._download_resource(id=sb_resource.id))

        if self.task:
            if len(resource_list) > 0:
                resource_list_str = "".join("<li>%s</li>" % res for res in resource_list)
                self.task.set_info('Loaded resources:<ul>{}</ul>'.format(resource_list_str), do_escape=False)

        return placeholder

    def update_ya_token(self, env):
        if 'YA_TOKEN' not in env and 'ARC_TOKEN' in env:
            env['YA_TOKEN'] = env['ARC_TOKEN']

    @contextlib.contextmanager
    def mount_arc(self):
        if not self.input.config.arc_mount_config.enabled:
            yield {}
        else:
            if self.input.config.arc_mount_config.arc_token.uuid or self.input.config.arc_mount_config.arc_token.key:
                arc_token_spec = self.input.config.arc_mount_config.arc_token
                self._setup_uuid(arc_token_spec)
            else:
                assert self.input.context.secret_uid, 'Can\'t specify ARC_TOKEN'
                arc_token_spec = yav.YavSecretSpec(uuid=self.input.context.secret_uid, key='ci.token')
            arc_token = self.ctx.yav.get_secret(arc_token_spec).secret

            yav_token_spec = self.input.config.arc_mount_config.yav_token
            if yav_token_spec.uuid or yav_token_spec.key:
                yav_token = self.ctx.yav.get_secret(yav_token_spec).secret
            else:
                yav_token = None

            extra_params = list(self.input.config.arc_mount_config.extra_params)

            arc_branch = 'trunk'
            if self.input.config.arc_mount_config.revision_hash:
                arc_branch = self.input.config.arc_mount_config.revision_hash
            elif self.input.context.target_revision.hash:
                arc_branch = self.input.context.target_revision.hash

            arc = lib_arc.Arc(arc_oauth_token=arc_token, yav_oauth_token=yav_token)
            with arc.mount_path('', arc_branch, mount_point=ARCADIA_PATH, fetch_all=False,
                                extra_params=extra_params):
                arc_binary_path = arc.binary_path
                arcadia_path = os.path.abspath(ARCADIA_PATH)

                path = os.path.dirname(arc_binary_path) + os.pathsep + arcadia_path + os.pathsep + os.environ["PATH"]

                env = {
                    'ARC_TOKEN': arc_token,
                    'ARC_BIN': arc_binary_path,
                    'ARCADIA_PATH': arcadia_path,
                    'PATH': path
                }

                yield env

    @contextlib.contextmanager
    def process_log(self):
        if self.task:
            with sandbox_sdk2.helpers.ProcessLog(logger='run_command', formatter=self._get_logs_formatter()) as pl:
                logging.info('stdout and stderr redirected to local files')
                resource = self._get_task_logs_resource()
                if resource is not None:
                    resource_id = resource.id

                    configs = self._get_logs_configs()
                    for config in configs:
                        id = config['id']
                        file = config['file']
                        path = self._get_proxy_resource_link(resource_id, file)
                        logging.info('{}: {}'.format(id, path))
                        self.task.set_info('{}: <a href="{}">{}</a>'.format(id, path, file), do_escape=False)

                        if config['ci_badge']:
                            self.save_logs_badge(resource, config['id'], config['text'], config['file'])

                yield pl
        else:
            logging.info('stdout and stderr redirected to console')
            yield sys

    def run_command(self, cmd, env):
        logging.info('Run command "%s"', cmd)
        with self.process_log() as pl:
            cwd = env.get('ARCADIA_PATH', os.getcwd())
            process = subprocess.Popen(
                cmd,
                env=env,
                cwd=cwd,
                shell=True,
                stdout=pl.stdout,
                stderr=self._get_stderr(pl.stderr),
                close_fds=True,
            )
            process.wait()
        rc = process.returncode
        logging.info('Command finished with return code: {}'.format(rc))
        self.output.state.return_code = rc
        if rc != 0:
            self.add_error('Command failed with return code: {}'.format(rc))

    def save_resources(self):
        for result_resource in self.input.config.result_resources:
            resource_path = os.path.join(RESULT_RESOURCES_PATH, result_resource.path)
            if not os.path.exists(resource_path):
                if not result_resource.optional:
                    self.add_error('Result resource path {} not found'.format(result_resource.path))
                continue
            compression_type = result_resource.compression_type or DEFAULT_COMPRESSION_TYPE
            compression_type = compression_type.lower()

            attrs = {'pack_tar': COMPRESSION_TYPE_RESOLVING[compression_type]}
            for key in result_resource.attributes:
                attrs[key] = result_resource.attributes[key]

            # Override hardcoded resources from file
            if result_resource.attributes_path:
                attributes_path = os.path.join(RESULT_RESOURCES_PATH, result_resource.attributes_path)
                if not os.path.exists(attributes_path):
                    self.add_error('Result resource attributes path {} not found'.format(result_resource.attributes_path))
                    continue

                with open(attributes_path, 'r') as f:
                    attrs_from_file = json.load(f)

                for key in attrs_from_file:
                    attrs[key] = attrs_from_file[key]

            resource = sandbox_sdk2.Resource[result_resource.type](
                self.task,
                result_resource.description,
                resource_path,
                None,
                **attrs
            )

            pb_resources = self.output.resources.add()
            pb_resources.id = resource.id
            pb_resources.task_id = resource.task_id
            pb_resources.type = str(result_resource.type)

            for key in attrs:
                pb_resources.attributes[key] = str(attrs[key])

            if result_resource.ci_badge:
                progress = ci.TaskletProgress()
                progress.job_instance_id.CopyFrom(self.input.context.job_instance_id)
                progress.id = result_resource.path
                progress.progress = 1.0
                progress.text = result_resource.description or "Uploaded {}".format(result_resource.path)
                progress.module = "SANDBOX"
                progress.url = self._get_resource_link(resource.id, result_resource.ci_badge_path or None)
                progress.status = ci.TaskletProgress.Status.SUCCESSFUL
                progress.ci_env = get_ci_env(self.input.context)
                self.ctx.ci.UpdateProgress(progress)

            data = sandbox_sdk2.ResourceData(resource)
            data.ready()

    def save_external_resources(self):
        if not self.input.config.result_external_resources_from_files:
            return

        for resource_id in os.listdir(RESULT_EXTERNAL_RESOURCES_PATH):
            if not resource_id.isnumeric():
                self.add_error('Invalid external resource id {}'.format(resource_id))
                continue
            resource = self._find_resource(id=resource_id)

            pb_resources = self.output.resources.add()
            pb_resources.id = resource.id
            pb_resources.task_id = resource.task_id
            pb_resources.type = str(resource.type)

            progress = ci.TaskletProgress()
            progress.job_instance_id.CopyFrom(self.input.context.job_instance_id)
            progress.id = 'ext_resource_{}'.format(resource.id)
            progress.progress = 1.0
            progress.text = resource.description or "External resource {}".format(resource.type)
            progress.module = "SANDBOX"
            progress.url = self._get_resource_link(resource.id)
            progress.status = ci.TaskletProgress.Status.SUCCESSFUL
            progress.ci_env = get_ci_env(self.input.context)
            self.ctx.ci.UpdateProgress(progress)

    def save_output(self):
        for result_output in self.input.config.result_output:
            resource_path = os.path.join(RESULT_RESOURCES_PATH, result_output.path)
            if not os.path.exists(resource_path):
                self.add_error('Result output path {} not found'.format(result_output.path))
                continue

            pb_result_output = self.output.result_output.add()
            pb_result_output.path = result_output.path

            with open(resource_path, 'r') as resource_file:
                while True:
                    line = resource_file.readline()
                    if not line:
                        break
                    line = line.strip()
                    if line:
                        pb_result_output.string.append(line)

    def save_badges(self):
        for result_badge in self.input.config.result_badges:
            resource_path = os.path.join(RESULT_BADGES_PATH, result_badge.path)
            if not os.path.exists(resource_path):
                self.add_error('Result badge path {} not found'.format(result_badge.path))
                continue

            with open(resource_path, 'r') as resource_file:
                while True:
                    line = resource_file.readline()
                    if not line:
                        break
                    line = line.strip()
                    if line:
                        progress = Parse(line, ci.TaskletProgress())
                        progress.job_instance_id.CopyFrom(self.input.context.job_instance_id)
                        progress.progress = 1.0
                        progress.ci_env = get_ci_env(self.input.context)
                        self.ctx.ci.UpdateProgress(progress)

    def save_logs_badge(self, resource, id, text, filename):
        progress = ci.TaskletProgress()
        progress.job_instance_id.CopyFrom(self.input.context.job_instance_id)
        progress.id = id
        progress.progress = 1.0
        progress.text = text
        progress.module = "SANDBOX"
        progress.url = self._get_proxy_resource_link(resource.id, filename)
        progress.status = ci.TaskletProgress.Status.SUCCESSFUL
        progress.ci_env = get_ci_env(self.input.context)
        self.ctx.ci.UpdateProgress(progress)

    def add_error(self, msg):
        self.task.set_info('<b style="color:red">{}</b>'.format(msg), do_escape=False)
        self.errors.append(msg)

    def check_for_errors(self):
        if len(self.errors) > 0:
            self.output.state.message = ", ".join(self.errors)
            raise Exception("Tasklet failed")
