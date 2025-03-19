#!/usr/bin/env python3
# -*- coding: utf-8 -*-
# pylint: skip-file
import logging
import socket
import time
from datetime import timedelta
from threading import Thread
from traceback import format_exc
from uuid import uuid4

from flask import jsonify
from marshmallow import Schema, validate
from marshmallow.fields import (Bool, Dict, Float, Function, Int, List, Nested, Str)
from webargs import core
from webargs.flaskparser import use_kwargs

import arbiter
from server_mock import ServerMock, check_auth

APP = ServerMock('fake_dbm')
ARBITER = arbiter.ArbiterClient(
    command_file='/arbiter/commands.txt',
    status_file='/arbiter/status.txt',
)
REGISTRY = dict()
TASKS = dict()


class Task:
    def __init__(self, function, args):
        self.done = False
        self.error = None
        self.function = function
        self.args = args

    def run(self):
        try:
            self.function(*self.args)
        except Exception:
            self.error = format_exc()
        self.done = True


@APP.reset_handler
def drop_state():
    """
    Cleanup all in-memory state
    """
    REGISTRY.clear()


def _parse_bootstrap_cmd(value):
    bootstraps = APP.config['BOOTSTRAPS']
    try:
        return bootstraps[value]
    except KeyError:
        raise core.ValidationError('unsupported {}, must be one of {}'.format(value, ','.join(bootstraps.keys())))


def _unit_to_int(value):
    """
    Convert stuff like "10 Kb" to 10240
    """
    if not isinstance(value, str):
        return value

    units = {
        'k': 1024,
        'm': 1024**2,
        'g': 1024**3,
        't': 1024**4,
        'p': 1024**5,
    }
    unit = ''
    number = value
    for i, char in enumerate(value):
        char = char.lower()
        if char in units:
            unit = char
            number = value[:i]
            break
    try:
        return int(number) * units.get(unit, 1)
    except (ValueError, TypeError):
        raise core.ValidationError('invalid value: {0}'.format(value))


class Volume(Schema):
    """
    Volume modify options
    """

    class Meta:
        strict = True

    space_guarantee = Function(deserialize=_unit_to_int, missing=None)
    space_limit = Function(deserialize=_unit_to_int, required=True)
    inode_guarantee = Int(missing=None)
    inode_limit = Int(missing=None)


class VolumeCreate(Volume):
    """
    Volume create options
    """
    path = Str(missing=None, required=True)
    dom0_path = Str(missing=None)
    backend = Str(validate=validate.Equal('native'), missing='native')


class Container(Schema):
    """
    Container modify options
    """

    class Meta:
        strict = True

    cpu_guarantee = Float(missing=None)
    cpu_limit = Float(missing=None)
    generation = Int(missing=None)
    hugetlb_limit = Int(missing=None)
    io_limit = Function(deserialize=_unit_to_int, missing=None)
    memory_guarantee = Function(deserialize=_unit_to_int, missing=None)
    memory_limit = Function(deserialize=_unit_to_int, missing=None)
    net_guarantee = Function(deserialize=_unit_to_int, missing=None)
    net_limit = Function(deserialize=_unit_to_int, missing=None)


class ContainerCreate(Container):
    """
    Container create options
    """
    project = Str(
        required=True,
        validate=validate.OneOf(['sandbox', 'pers', 'pgaas', 'disk']),
        missing=None,
    )
    cluster = Str(required=True)
    dom0 = Str(
        required=False,
        default='fake-dom0',
        missing=None,
    )
    image_type = Function(
        deserialize=_parse_bootstrap_cmd,
        required=True,
        load_from='bootstrap_cmd',
    )
    extra_properties = Dict(required=False, missing=dict())
    secrets = Dict(required=False, missing=dict())
    volumes = List(Nested(VolumeCreate))


def _resolve_ip6(hostname):
    """
    Resolve hostname
    """
    addr, *_ = socket.getaddrinfo(hostname, 0, socket.AF_INET6)
    # >>> addr
    # (<AddressFamily.AF_INET6: 10>,
    #  <SocketKind.SOCK_STREAM: 1>,
    #  6,
    #  '',
    #  ('fd00:dead:beef:28a8:47b5:461f:0:14', 0, 0, 0))
    return addr[4][0]


def _delete_container(fqdn):
    """
    Shut the container down and remove it.
    Assumes containers were not created in persistent mode
    """
    name, *_ = fqdn.partition('.')
    return ARBITER.run('container_remove', name=name)


def _stop_container(fqdn):
    """
    Shut the container down.
    """
    name, *_ = fqdn.partition('.')
    return ARBITER.run('container_stop', name=name)


def _create_container(fqdn, cont_type, secrets=None):
    """
    Creates a container node in configuration,
    calls renderers and compose-start step
    """
    if secrets is None:
        secrets = dict()
    name, *_ = fqdn.partition('.')
    domain = APP.config['DBM']['network_name']
    hosts = []
    fakes = {
        'c.yandex-team.ru': _resolve_ip6('fake_conductor01.{0}'.format(domain)),
        'ro.admin.yandex-team.ru': '::1',
    }
    for real_fqdn, ip in fakes.items():
        hosts.append('{name}:{ip}'.format(name=real_fqdn, ip=ip))
    expose = APP.config['EXPOSES'].get(cont_type, {})
    print('running {name} {cont_type} container (expose {expose} ports)'.format(**locals()))

    ARBITER.run(
        'merge_and_exec',
        conf={
            'projects': {
                name: {
                    'privileged': True,
                    'instance_name': name,
                    'image': 'dbaas-infra-tests-{type}:{tag}'.format(
                        type=cont_type,
                        tag=domain,
                    ),
                    'build': 'skip',
                    'docker_instances': 1,
                    'extra_hosts': hosts,
                    'expose': expose,
                },
            },
        },
        steps=[
            'tests.helpers.compose.create_config',
            'tests.helpers.compose.startup_containers',
        ],
    )
    for path, props in secrets.items():
        ARBITER.run(
            'container_put',
            name=fqdn,
            path=path,
            content=props['content'],
            mode=props['mode'],
        )
    ARBITER.run(
        'container_put',
        name=fqdn,
        path='/etc/apt/apt.conf.d/10useip6',
        content='Acquire::ForceIPv6 "true";',
    )


def _get_volumes(fqdn):
    """
    Get volume properties of container
    """
    volumes = []
    container = REGISTRY.get(fqdn)
    if container is None:
        return volumes
    for props in container.get('volumes', []):
        volume = props.copy()
        volume.update({
            'dom0': container.get('dom0'),
            'container': fqdn,
            'read_only': False,
        })
        volumes.append(volume)
    return volumes


def worker():
    """
    Our separate thread worker
    """
    while True:
        for task in TASKS.copy().values():
            if not task.done:
                task.run()
        time.sleep(0.1)


WORKER = Thread(target=worker, daemon=True)
WORKER.start()


@APP.route('/api/v2/operations/<operation_id>')
@check_auth
def get_operation(operation_id):
    """
    Return info on operation
    """
    if operation_id in TASKS:
        task = TASKS[operation_id]
        return jsonify({'done': task.done, 'error': task.error})

    res = jsonify({'error': 'No such operation'})
    res.status_code = 404
    return res


@APP.route('/api/v2/transfers/')
@use_kwargs({'fqdn': Str(required=True)}, locations=('query', ))
@check_auth
def get_transfers(fqdn):
    """
    Return info on container
    """
    return jsonify({})


@APP.route('/api/v2/containers/<fqdn>')
@check_auth
def get_container(fqdn):
    """
    Return info on container
    """
    if fqdn in REGISTRY:
        return jsonify(REGISTRY[fqdn]['container'])

    res = jsonify({'error': 'No such container'})
    res.status_code = 404
    return res


@APP.route('/api/v2/containers/<fqdn>', methods=['POST'])
@use_kwargs(Container(), locations=('json', ))
@check_auth
def modify_container(fqdn, **kwargs):
    """
    Change container options.
    Currently only changes dict in REGISTRY
    """
    container = REGISTRY.get(fqdn)
    if not container:
        res = jsonify({'error': 'No such container'})
        res.status_code = 404
        return res

    REGISTRY[fqdn]['container'].update({key: val for key, val in kwargs.items() if val is not None})
    task_id = str(uuid4())
    TASKS[task_id] = Task(logging.info, ('Noop on %s modification', fqdn))
    return jsonify({'operation_id': task_id})


@APP.route('/api/v2/containers/<fqdn>', methods=['PUT'])
@use_kwargs(ContainerCreate(), locations=('json', ))
@check_auth
def create_container(fqdn, **kwargs):
    """
    Creates a docker container with a specified hostname
    and puts it into a registry.
    """
    if REGISTRY.get(fqdn):
        res = jsonify({'error': 'Container already exists'})
        res.status_code = 400
        return res
    REGISTRY[fqdn] = {
        'container': {k: v
                      for k, v in kwargs.items() if k not in ['image_type', 'volumes']},
        'volumes': kwargs['volumes'],
    }
    task_id = str(uuid4())
    TASKS[task_id] = Task(_create_container, (fqdn, kwargs['image_type'], kwargs['secrets']))
    return jsonify({'operation_id': task_id})


@APP.route('/api/v2/containers/<fqdn>', methods=['DELETE'])
@check_auth
def delete_container(fqdn):
    """
    Shut the container down and remove it from registry
    """
    if fqdn in REGISTRY:
        del REGISTRY[fqdn]
        task_id = str(uuid4())
        TASKS[task_id] = Task(_stop_container, (fqdn, ))
        return jsonify({'operation_id': task_id})

    res = jsonify({'error': 'No such container'})
    res.status_code = 404
    return res


@APP.route('/api/v2/volumes/<fqdn>', methods=['GET'])
@check_auth
def list_volumes(fqdn):
    """
    List container volumes
    """
    container = REGISTRY.get(fqdn)
    if not container:
        return jsonify([])
    return jsonify(container['volumes'])


@APP.route('/api/v2/volumes/<fqdn>', methods=['POST'])
@use_kwargs(Volume(), locations=('json', ))
@use_kwargs({
    'path': Str(required=True),
    'init_deploy': Bool(missing=True),
}, locations=('query', ))
@check_auth
def modify_volume(fqdn, path, init_deploy, **opts):
    """
    Update volume`s options
    """
    container = REGISTRY.get(fqdn)
    if container is None:
        res = jsonify({'error': 'No such container'})
        res.status_code = 404
        return res

    volume = None
    for vol in container['volumes']:
        if vol['path'] == path:
            volume = vol
    if volume is None:
        res = jsonify({'error': 'No such volume'})
        res.status_code = 404
        return res
    volume.update({key: value for key, value in opts.items() if value is not None})
    if init_deploy:
        task_id = str(uuid4())
        TASKS[task_id] = Task(logging.info, ('Noop on %s volume %s modification', fqdn, path))
        return jsonify({'operation_id': task_id})
    else:
        return jsonify({})


@APP.route('/ping')
def ping():
    """
    Slb check
    """
    if not WORKER.is_alive():
        res = jsonify(status='Worker not running')
        res.status_code = 500
        return res

    return jsonify(status='OK')
