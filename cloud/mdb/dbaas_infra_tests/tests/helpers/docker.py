"""
Docker-helpers for dbaas infra tests.
This module defines functions that facilitate the interaction with docker,
e.g. creating or shutting down an external network.
"""

import io
import json
import os
import random
import subprocess
import tarfile
import time
from concurrent.futures import ThreadPoolExecutor
from multiprocessing import cpu_count
from threading import Lock
from typing import Sequence

import docker
from docker.errors import NotFound
from docker.models.containers import Container
from humanfriendly import format_timespan

from . import utils

STDOUT_LOCK = Lock()
DOCKER_API = docker.from_env()


def run_command(name: str, cmd: str, user: str = 'root', environment=None) -> (int, str):
    """
    Runs specified command inside container, returns retcod and output
    Stdout and stderr are mixed
    """
    container = DOCKER_API.containers.get(name)
    return container.exec_run(cmd, stdout=True, stderr=True, user=user, environment=environment)


def get_file(name: str, filepath: str) -> str:
    """
    Returns the file's content from the container's FS
    """
    container = DOCKER_API.containers.get(name)
    raw_tar, stat = container.get_archive(filepath)
    fileobj = io.BytesIO(b''.join(raw_tar))
    with tarfile.open(mode='r', fileobj=fileobj) as tar:
        return tar.extractfile(stat['name']).read().decode()


def put_file(name, filepath, content, mode=0o0600):
    """
    Puts file into containers FS
    """
    container = DOCKER_API.containers.get(name)
    # convert file to byte via BytesIO
    infile = io.BytesIO(content.encode())
    path, name = os.path.split(filepath)

    # create tar archive
    tar_archive = io.BytesIO()
    tar = tarfile.open(mode='w', fileobj=tar_archive)
    tarinfo = tarfile.TarInfo(name)
    tarinfo.size = len(content)
    tarinfo.mode = mode
    tar.addfile(tarinfo, infile)
    tar.close()
    container.put_archive(path, tar_archive.getvalue())


def get_containers(conf: dict) -> Sequence[Container]:
    """
    Get container objects.
    """
    containers = []
    for raw_container in DOCKER_API.api.containers(all=True):
        try:
            container = DOCKER_API.containers.get(raw_container['Id'])
            if container.attrs['Config']['Hostname'].endswith(conf['network_name']):
                containers.append(container)
        except NotFound:
            pass

    return containers


def get_container(context, prefix: str) -> Container:
    """
    Get container object by prefix
    """
    return DOCKER_API.containers.get('%s.%s' % (prefix, context.conf['network_name']))


def get_exposed_port(container, port):
    """
    Get pair of (host, port) for connection to exposed port
    """
    host = 'localhost'
    machine_name = os.getenv('DOCKER_MACHINE_NAME')
    if machine_name:
        host = subprocess.check_output(['docker-machine', 'ip', machine_name]).decode('utf-8').rstrip()

    binding = container.attrs['NetworkSettings']['Ports'].get('{0}/tcp'.format(port))

    if binding:
        return (host, binding[0]['HostPort'])
    raise RuntimeError('port {0} binding for container {1} not found'.format(port, container.name))


def iterate_build_log_as_text(build_log):
    """
    Expand sequence of dicts of log to iterable str

    :param build_log: Iterable[dict]
    :return: Iterable[str]
    """
    for line in build_log:
        if 'stream' in line:
            yield line['stream']
        else:
            yield json.dumps(line)


def generate_ipv6():
    """
    Generates a random IPv6 subnet.
    """
    subnet = 'fd00:dead:beef:%s::/96'
    random_part = ':'.join(['%x' % random.randint(0, (16**4 - 1)) for _ in range(3)])
    return subnet % random_part


def generate_ipv4():
    """
    Generates a random IPv4 subnet.
    """
    subnet = '10.%s.0/24'
    random_part = '.'.join(['%d' % random.randint(0, 254) for _ in range(2)])
    return subnet % random_part


def build_base_image(props):
    """
    Build base docker image
    """
    try:
        start_time = time.time()
        # Use bridge because we have parallel build with port usage.
        DOCKER_API.images.build(network_mode="bridge", **props)
        build_time = format_timespan(time.time() - start_time)
        with STDOUT_LOCK:
            print(f'Building {props["path"]} as {props["tag"]} took {build_time}', flush=True)
    except Exception as exc:
        raise RuntimeError('container {0} build failed: {1}'.format(str(props), exc))


def build_image(name, props, staging, net_name):
    """
    Build regular docker image
    """
    build = props.get('build')
    if not build:
        return
    image_path = os.path.join(staging, build['path'])
    if build['path'].startswith('/'):
        image_path = build['path']
    image_tag = build.get('tag', f'{name}:{net_name}')

    for cmd in props.get('prebuild_cmd', []):
        try:
            start_time = time.time()
            proc = subprocess.run([cmd], shell=True, check=True, stdout=subprocess.PIPE, stderr=subprocess.PIPE)
            build_time = format_timespan(time.time() - start_time)
            with STDOUT_LOCK:
                print(
                    f'Prebuild cmd {cmd} took {build_time}, '
                    f'stdout: {proc.stdout.decode("utf-8")}, '
                    f'stderr: {proc.stderr.decode("utf-8")}',
                    flush=True)
        except subprocess.CalledProcessError as err:
            raise RuntimeError(f'prebuild command {cmd} failed: {err}\n'
                               f'stdout: {err.stdout}\n'
                               f'stderr: {err.stderr}')

    try:
        start_time = time.time()
        # Use bridge because we have parallel build with port usage.
        DOCKER_API.images.build(
            path=image_path,
            tag=image_tag,
            pull=build.get('pull', True),
            network_mode="bridge",
        )
        build_time = format_timespan(time.time() - start_time)
        print(f'Building {image_path} as {image_tag} took {build_time}', flush=True)
    except docker.errors.BuildError as err:
        build_log = '\n'.join(iterate_build_log_as_text(err.build_log))
        raise RuntimeError(f'container {name} build failed: {err}\n'
                           'you can debug it with: '
                           f'docker build --pull {image_path} -t {image_tag}\n'
                           'or read build log:\n'
                           f'{build_log}')


@utils.env_stage('create', fail=True)
def build_images(state, conf):
    """
    Build images.
    """
    for arg in (state, conf):
        assert arg is not None, f'{arg} must not be None'
    with ThreadPoolExecutor(max_workers=cpu_count(), thread_name_prefix='image-build-') as build_pool:
        base_futures = []
        for props in conf.get('base_images', {}).values():
            base_futures.append(build_pool.submit(build_base_image, props))

        for future in base_futures:
            future.result()

        image_futures = []
        staging = conf['staging_dir']
        net_name = conf['network_name']
        for name, props in conf['projects'].items():
            image_futures.append(build_pool.submit(build_image, name, props, staging, net_name))

        for future in image_futures:
            future.result()


@utils.env_stage('create', fail=True)
def prep_network(state, conf):
    """
    Creates ipv6-enabled docker network with random name and address space
    """
    for arg in (state, conf):
        assert arg is not None, '%s must not be None' % arg

    # Unfortunately docker is retarded and not able to create
    # ipv6-only network (see https://github.com/docker/libnetwork/issues/1192)
    # Do not create new network if there is an another net with the same name.
    if DOCKER_API.networks.list(names='^%s$' % conf['network_name']):
        return
    ip_subnet_pool = docker.types.IPAMConfig(pool_configs=[
        docker.types.IPAMPool(subnet=conf['dynamic']['docker_ip4_subnet']),
        docker.types.IPAMPool(subnet=conf['dynamic']['docker_ip6_subnet']),
    ])
    bridge_name = '{name}_{num}'.format(
        name=conf.get('docker_bridge_name', 'dbaas'),
        num=random.randint(0, 65535),
    )
    net_name = conf['network_name']
    net_opts = {
        'com.docker.network.bridge.enable_ip_masquerade': 'true',
        'com.docker.network.bridge.enable_icc': 'true',
        'com.docker.network.bridge.name': bridge_name,
    }
    DOCKER_API.networks.create(
        net_name,
        options=net_opts,
        enable_ipv6=True,
        ipam=ip_subnet_pool,
    )


@utils.env_stage('stop', fail=False)
def shutdown_network(conf, **_extra):
    """
    Stop docker network(s)
    """
    nets = DOCKER_API.networks.list(names=conf['network_name'])
    for net in nets:
        net.remove()


@utils.env_stage('stop', fail=False)
def remove_containers(conf, **_):
    """
    Remove any remaining containers, including created without docker-compose
    """
    for container in get_containers(conf):
        container.remove(force=True, v=True)
