from . import constants

from sandbox.common.errors import TaskFailure

import logging
import os
import yaml

LOGGER = logging.getLogger('config_helper')


class RunnerArtifactConfig(object):

    def __init__(self, config):
        LOGGER.debug('RunnerArtifactConfig.__init__ {}'.format(config))
        self.source_path = config['source_path']
        self.source_type = config['source_type']
        self.destination_path = config['destination_path']
        self.destination_type = config['destination_type']


class RunnerDependentArtifactConfig(object):

    def __init__(self, config):
        LOGGER.debug('RunnerDependentArtifactConfig.__init__{}'.format(config))
        self.type = config['type']
        self.parameter_name = config['parameter_name']


class RunnerDependentTaskConfig(object):

    def __init__(self, config):
        LOGGER.debug('RunnerDependentTaskConfg.__init__ {}'.format(config))
        self.name = config['name']
        self.artifacts = [RunnerDependentArtifactConfig(artifact_config) for artifact_config in config['artifacts']]


class RunnerTaskConfig(object):

    def __init__(self, config):
        LOGGER.debug('RunnerTaskConfig.__init__ {}'.format(config))
        self.name = config['name']
        self.config_path = config['config_path']
        self.artifacts = [RunnerArtifactConfig(artifact_config) for artifact_config in config['artifacts']]
        self.depends_on = [RunnerDependentTaskConfig(dependent_config) for dependent_config in config['depends_on']]


class RunnerConfig(object):

    def __init__(self, config):
        LOGGER.debug('RunnerConfig.__init__ {}'.format(config))
        self.task_configs = [RunnerTaskConfig(task_config) for task_config in config['task_configs']]


class TaskRequirementsConfig(object):

    def __init__(self, **dict_config):
        LOGGER.debug('TaskRequirementsConfig.__init__ {}'.format(dict_config))
        self.__dict__.update(dict_config)


class TaskParametersConfig(object):

    def __init__(self, **dict_config):
        LOGGER.debug('TaskParametersConfig.__init__ {}'.format(dict_config))
        self.__dict__.update(dict_config)


class TaskConfig(object):

    def __init__(self, config):
        LOGGER.debug('TaskConfig.__init__ {}'.format(config))
        self.task_type = config['task_type']
        self.requirements = TaskRequirementsConfig(**config['requirements'])
        self.parameters = TaskParametersConfig(**config['parameters'])


def is_subdir(parent_path, child_path):
    return parent_path == os.path.commonprefix(
        [
            parent_path,
            child_path,
        ]
    )


def get_config(config_path):
    try:
        LOGGER.debug('Opening config file with path: {}'.format(config_path))
        with open(config_path, 'r') as f:
            LOGGER.debug('Reading config with path: {}'.format(config_path))
            return f.read()
    except Exception as e:
        raise TaskFailure(e)


def read_config(root_path, config_path, schema):
    LOGGER.debug('read_config %s from root_path %s', config_path, root_path)

    current_path = config_path
    configs = list()
    while os.path.abspath(current_path) != os.path.abspath(root_path):
        configs.append(current_path)
        current_path = os.path.abspath(os.path.join(current_path, os.pardir))
    configs.append(current_path)
    configs.reverse()

    from deepmerge import always_merger

    config = dict()
    for config_path in configs:
        current_config_path = config_path
        if os.path.isdir(current_config_path):
            current_config_path = os.path.join(
                current_config_path,
                constants.DEFAULT_CONFIG_COMMON_FILE,
            )
            if not os.path.isfile(current_config_path):
                continue
        current_config = yaml.safe_load(get_config(current_config_path))
        config = always_merger.merge(config, current_config)

    return config


def validate_config(config, schema):
    LOGGER.debug('Try to validate config')

    from jsonschema import validate, ValidationError, SchemaError

    try:
        validate(config, schema)
    except (ValidationError, SchemaError) as e:
        raise TaskFailure(e)


def read_runner_config(root_path, config_path):
    if not is_subdir(root_path, config_path):
        raise TaskFailure(
            'Config {} is not inside root {}'.format(
                config_path,
                root_path,
            )
        )

    LOGGER.debug('Reading runner config from repository')
    config = read_config(
        root_path,
        config_path,
        constants.DEFAULT_CONFIG_SCHEMA_RUNNER,
    )
    LOGGER.debug('Got raw config: %s', config)

    from deepmerge import always_merger

    result_config = {'task_configs': list()}
    for task in config['task_configs']:
        indices = [i for i, x in enumerate(result_config['task_configs']) if x['name'] == task['name']]
        if len(indices) > 0:
            always_merger.merge(result_config['task_configs'][indices[0]], task)
        else:
            result_config['task_configs'].append(task)
    LOGGER.debug('Runner config %s has successfully been read', config)

    validate_config(result_config, constants.DEFAULT_CONFIG_SCHEMA_RUNNER)
    return RunnerConfig(result_config)


def read_task_config(root_path, config_path):
    if not is_subdir(root_path, config_path):
        raise TaskFailure(
            'Config %s is not inside root %s',
            config_path,
            root_path,
        )

    LOGGER.debug('Reading task config from repository')
    config = read_config(
        root_path,
        config_path,
        constants.DEFAULT_CONFIG_SCHEMA_TASK,
    )
    LOGGER.debug('Got raw config: %s', config)

    LOGGER.debug('Task config %s has successfully been read', config)

    validate_config(config, constants.DEFAULT_CONFIG_SCHEMA_TASK)
    return TaskConfig(config)
