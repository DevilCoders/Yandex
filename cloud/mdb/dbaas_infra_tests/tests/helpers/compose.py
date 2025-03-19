"""
Docker Compose helpers
"""

import copy
import os
import random
import shlex
import subprocess
from collections.abc import Iterable, Mapping

import yaml
from retrying import retry

from . import utils

# Default invariant config
BASE_CONF = {
    'version': '2',
    'networks': {
        'test_net': {
            'external': {
                'name': '{network}',
            },
        },
    },
    'services': {},
}


@utils.env_stage('create', fail=True)
def build_images(conf, **_extra):
    """
    Build docker images.
    """
    _check_args(conf=conf)

    _call_compose(conf, 'build')


@utils.env_stage('start', fail=True)
def startup_containers(conf, **_extra):
    """
    Start up docker containers.
    """
    _check_args(conf=conf)

    _call_compose(conf, 'up -d --no-recreate')


def restart_containers(conf, projects=None, **_extra):
    """
    Restart docker containers.
    """
    _check_args(conf=conf, projects=projects)

    command = 'restart'
    if projects:
        command += ' ' + ' '.join(_get_service_names(conf['projects'], *projects))

    _call_compose(conf, command)


@utils.env_stage('restart', fail=True)
def recreate_containers(conf, projects=None, **_extra):
    """
    Recreate and restart docker containers.
    """
    _check_args(conf=conf, projects=projects)

    command = 'up -d --force-recreate'
    if projects:
        command += ' ' + ' '.join(_get_service_names(conf['projects'], *projects))

    _call_compose(conf, command)


def remove_orphans(conf):
    """
    Remove all non-static containers
    """
    command = 'up -d --remove-orphans'
    _call_compose(conf, command)


@utils.env_stage('stop', fail=False)
def shutdown_containers(conf, **_extra):
    """
    Shutdown and remove docker containers.
    """
    _check_args(conf=conf)

    _call_compose(conf, 'down')


@utils.env_stage('create', fail=True)
def create_config(conf, **_extra):
    """
    Generate config file and write it.
    """
    _check_args(conf=conf)

    staging_dir = _get_staging_dir(conf)
    compose_conf_path = _get_config_path(conf, staging_dir)
    # Create this directory now, otherwise if docker does
    # it later, it will be owned by root.
    compose_conf = _generate_config_dict(
        projects=conf.get('projects', {}),
        network_name=conf['network_name'],
        basedir=staging_dir,
    )
    return _write_config(compose_conf_path, compose_conf)


def read_config(conf):
    """
    Reads compose config into dict.
    """
    _check_args(conf=conf)

    with open(_get_config_path(conf)) as conf_file:
        return yaml.load(conf_file)


def _check_args(**kwargs):
    if 'conf' in kwargs:
        conf = kwargs['conf']
        assert conf and isinstance(conf, Mapping), '"conf" must be non-empty dict'

    projects = kwargs.get('projects')
    if projects:
        assert isinstance(projects, Iterable), '"projects" must be iterable'


def _get_service_names(projects_config, *projects):
    """
    Return a list of docker-compose service names for one or more projects.
    """
    service_names = []
    for project in projects:
        instance_name_override = projects_config[project].get('instance_name')
        # Some containers require explicit name to be set beforehand.
        if instance_name_override is not None:
            service_names.append(instance_name_override)
            continue
        count = projects_config[project].get('docker_instances', 1)
        service_names.extend('{0}{1:02d}'.format(project, n + 1) for n in range(count))
    return service_names


def _write_config(path, compose_conf):
    """
    Dumps compose config into a file in Yaml format.
    """
    assert isinstance(compose_conf, dict), 'compose_conf must be a dict'

    catalog_name = os.path.dirname(path)
    os.makedirs(catalog_name, exist_ok=True)
    temp_file_path = '{dir}/.docker-compose-conftest-{num}.yaml'.format(
        dir=catalog_name,
        num=random.randint(0, 100),
    )
    with open(temp_file_path, 'w') as conf_file:
        yaml.dump(
            compose_conf,
            stream=conf_file,
            default_flow_style=False,
            indent=4,
        )
    try:
        _validate_config(temp_file_path)
        os.rename(temp_file_path, path)
    except subprocess.CalledProcessError as err:
        raise RuntimeError('unable to write config: validation failed with %s' % err)
    # Remove config only if validated ok.
    _remove_config(temp_file_path)


def _get_staging_dir(conf):
    return conf.get('staging_dir', 'staging')


def _get_config_path(conf, staging_dir=None):
    """
    Return file path to docker compose config file.
    """
    if not staging_dir:
        staging_dir = _get_staging_dir(conf)
    return os.path.join(staging_dir, 'docker-compose.yml')


def _remove_config(path):
    """
    Removes a config file.
    """
    try:
        os.unlink(path)
    except FileNotFoundError:
        pass


def _validate_config(config_path):
    """
    Perform config validation by calling `docker-compose config`
    """
    with open(os.devnull, 'w') as devnull:
        _call_compose_on_config(
            config_path,
            '__config_test',
            # Suppress output as it prints the whole config to stdout
            'config',
            output=devnull,
        )


def _generate_config_dict(projects, network_name, basedir):
    """
    Create docker-compose.yml with initial images
    """
    assert isinstance(projects, dict), 'projects must be a dict'

    compose_conf = copy.deepcopy(BASE_CONF)
    # Set net name at global scope so containers will be able to reference it.
    compose_conf['networks']['test_net']['external']['name'] = network_name
    # Generate service config for each project`s instance
    # Also relative to config file location.
    for name, props in projects.items():
        # Skip local-only projects
        if props.get('localinstall', False):
            continue

        code_path = props.get('code_path', '')

        for instance_name in _get_service_names(projects, name):
            # Account for instance-specific configs, if provided.
            # 'More specific wins' semantic is assumed.
            service_props = utils.merge(
                copy.deepcopy(props),
                # A service may contain a key with instance
                # name -- this is assumed to be a more specific
                # config.
                copy.deepcopy(props.get(instance_name, {})))

            # Generate 'service' section configs.
            service_conf = _generate_service_dict(
                name=name,
                code_path=code_path,
                instance_name=instance_name,
                instance_conf=service_props,
                network=network_name,
                basedir=basedir,
            )
            # Fill in local placeholders with own context.
            # Useful when we need to reference stuff like
            # hostname or domainname inside of the other config value.
            service_conf = utils.format_object(service_conf, **service_conf)
            compose_conf['services'].update({instance_name: service_conf})
    return compose_conf


def _generate_service_dict(name, code_path, instance_name, instance_conf, network, basedir):
    """
    Generates a single service config based on name and
    instance config.

    All paths are relative to the location of compose-config.yaml
    (which is ./staging/compose-config.yaml by default)
    """

    # Take care of volumes
    if code_path:
        code_volume = './code/{code_path}'.format(code_path=code_path)
    else:
        code_volume = './code/{name}'.format(name=name)

    local_code_volume = './images/{name}/src'.format(name=name)
    # Override '/code' path if local code is present.
    if os.path.exists(os.path.join(basedir, local_code_volume)):
        code_volume = local_code_volume
    volumes = {
        # Source code -- the original cloned repository.
        'code': {
            'local': code_volume,
            'remote': '/code',
            'mode': 'rw',
        },
        # Instance configs from images
        'config': {
            'local': './images/{name}/config'.format(name=name),
            'remote': '/config',
            'mode': 'rw',
        },
        # Container-host communication
        'arbiter': {
            'local': './arbiter',
            'remote': '/arbiter',
            'mode': 'rw',
        },
    }
    volume_list = _prepare_volumes(
        volumes,
        local_basedir=basedir,
    )
    # Take care of port forwarding
    ports_list = []
    for port in instance_conf.get('expose', {}).values():
        ports_list.append(port)

    ret = {
        'image': instance_conf.get('image', '{nm}:{nt}'.format(nm=name, nt=network)),
        'hostname': '%s.%s' % (instance_name, network),
        'domainname': '',
        # Networks. We use external anyway.
        'networks': instance_conf.get('networks', ['test_net']),
        # Runtime envs
        'environment': instance_conf.get('environment', []),
        # Some containers, e.g. postgres templates require elevated
        # privileges to run Salt.
        'privileged': instance_conf.get('privileged', False),
        # Nice container name with domain name part.
        # This results, however, in a strange rdns name:
        # the domain part will end up there twice.
        # Does not affect A or AAAA, though.
        'container_name': '%s.%s' % (instance_name, network),
        # Ports exposure
        'ports': ports_list + instance_conf.get('ports', []),
        # Config and code volumes
        'volumes': volume_list + instance_conf.get('volumes', []),
        # external resolver: dns64-cache.yandex.net
        'dns': ['2a02:6b8:0:3400::1023'],
        # dns cname list
        # https://docs.docker.com/compose/compose-file/#external_links
        'external_links': instance_conf.get('external_links', []),
        # DNS aliases put into hosts file.
        'extra_hosts': instance_conf.get('extra_hosts', []),
    }
    if 'no_tmpfs' not in instance_conf:
        ret['tmpfs'] = '/var/run'
    return ret


def _prepare_volumes(volumes, local_basedir):
    """
    Form a docker-compose volume list,
    and create endpoints.
    """
    assert isinstance(volumes, dict), 'volumes must be a dict'

    volume_list = []
    for props in volumes.values():
        # "local" params are expected to be relative to
        # docker-compose.yaml, so prepend its location.
        os.makedirs(
            '{base}/{dir}'.format(
                base=local_basedir,
                dir=props['local'],
            ), exist_ok=True)
        volume_list.append('{local}:{remote}:{mode}'.format(**props))
    return volume_list


@retry(wait_random_min=1000, wait_random_max=2000, stop_max_attempt_number=10)
def _call_compose(conf, action):
    conf_path = '{base}/docker-compose.yml'.format(base=conf.get('staging_dir', 'staging'))
    project_name = conf['network_name']
    with open(os.devnull, 'w') as devnull:
        _call_compose_on_config(
            conf_path,
            project_name,
            # Suppress output as it prints the whole config to stdout
            action,
            output=devnull,
        )


@retry(wait_random_min=1000, wait_random_max=2000, stop_max_attempt_number=10)
def _call_compose_on_config(conf_path, project_name, action, output=None):
    """
    Execute docker-compose action by invoking `docker-compose`.
    """
    assert isinstance(action, str), 'action arg must be a string'

    compose_cmd = 'docker-compose --file {conf} -p {name} {action}'.format(
        conf=conf_path,
        name=project_name,
        action=action,
    )
    # Note: build paths are resolved relative to config file location.
    subprocess.check_call(shlex.split(compose_cmd), stdout=output, stderr=output)
