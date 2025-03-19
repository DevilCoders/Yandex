#!/usr/bin/env python
# -*- encoding: utf-8
"""
Application context. All routes are defined here
"""

import logging
import os
from functools import partial

import requests as _  # noqa, pylint: disable=unused-import
from dbaas_common import tracing
from flask import Flask, abort, g, jsonify, render_template
from flask_bootstrap import Bootstrap
from flask_opentracing import FlaskTracing
from humanfriendly import parse_size
from opentracing_instrumentation.client_hooks.requests import patcher  # type: ignore
from webargs import ValidationError, flaskparser, validate
from webargs.fields import Bool, Dict, Field, Float, Function, Int, Nested, Str

from . import generations
from .common import init_logging
from .config import app_config
from .containers import (
    destroy_container,
    drop_container,
    get_container,
    get_containers,
    get_pillar,
    launch_container,
    update_container,
)
from .db import clean_context, commit
from .dom0 import get_dom0_host, update_allow_new_hosts, update_dom0_host
from .metrics import init_metrics
from .passport import check_auth
from .sentry import init_sentry
from .transfers import cancel_transfer, finish_transfer, get_transfer, get_transfer_by_container, get_transfers
from .update_volume import update_volume
from .volume_backups import delete_volume_backup, drop_volume_backup, get_volume_backups
from .volumes import get_container_volumes, get_volumes

patcher.install_patches()


def resolve_static_path():
    """
    Resolve path to static folder
    """
    if 'ARCADIA_SOURCE_ROOT' in os.environ:
        return os.path.join(
            os.environ['ARCADIA_SOURCE_ROOT'],
            'cloud/mdb/dbm/internal/static',
        )
    return '/var/lib/yandex/dbm/static'


init_logging()
init_sentry()
APP = Flask(__name__, static_folder=resolve_static_path())
Bootstrap(APP)
APP.logger = logging.getLogger('flask.app')  # type:ignore
PARSER = flaskparser.FlaskParser()
init_metrics(APP)


try:
    from uwsgidecorators import postfork  # type:ignore
except ImportError:
    APP.logger.warning('uwsgi missing')
else:

    @postfork
    def init_tracing():
        """
        Initialize tracing
        """
        default = {
            'service_name': 'dbm',
            'disabled': False,
            'local_agent': {
                'reporting_host': 'localhost',
                'reporting_port': '6831',
            },
            'queue_size': 10000,
            'sampler': {
                'type': 'const',
                'param': 1,
            },
            'logging': False,
        }
        default.update(app_config().get('TRACING', {}))
        APP.tracer = tracing.init_tracing(default)
        if APP.tracer:
            APP.flask_tracer = FlaskTracing(APP.tracer, True, APP)
            tracing.init_requests_interceptor()
        else:
            log = logging.getLogger('common')
            log.warning('app.tracer missing')


# pylint: disable=unused-argument
@PARSER.error_handler
def jsonify_error(error, req, schema, status_code, headers):
    """
    Return error as json
    """
    log = logging.getLogger('flask.app')
    log.error(error)
    res = jsonify(error.normalized_messages())
    res.status_code = status_code if status_code else 422
    abort(res)


def size_deserialize(value):
    """
    Deserialize size with humanfriendly
    """
    if isinstance(value, int):
        return value
    try:
        return parse_size(value, binary=True)
    except Exception:
        # pylint: disable=raise-missing-from
        raise ValidationError(f'Invalid size: {value}')


def size_validator(value):
    """
    Check if size is valid human-size (1MB, 1KB, 1)
    """
    int_value = size_deserialize(value)
    if int_value and int_value < 0:
        raise ValidationError(f'Negative size: {value}')


SIZE_FIELD = partial(Function, location='json', validate=size_validator, deserialize=size_deserialize)


def search_query_validator(value):
    """
    Check if search query is valid
    """
    pairs = value.split(';')
    for pair in pairs:
        if not pair:
            continue
        if pair.count('=') != 1:
            raise ValidationError("Invalid query: '{value}'".format(value=value))
        key, value = pair.split('=')
        if not key or not value:
            raise ValidationError("Invalid pair: '{pair}'".format(pair=pair))


VOLUME: dict[str, Field] = {
    'read_only': Bool(location='json', missing=False),
    'space_guarantee': SIZE_FIELD(),
    'space_limit': SIZE_FIELD(),
    'inode_guarantee': Int(location='json', validate=validate.Range(min=0)),
    'inode_limit': Int(location='json', validate=validate.Range(min=0)),
}

VOLUME_CREATE: dict[str, Field] = {
    **VOLUME,
    'backend': Str(location='json', missing='native', validate=validate.OneOf(['native', 'tmpfs'])),
    'dom0_path': Str(location='json', required=True, validate=validate.Regexp('/[a-z0-9_/-]*')),
    'path': Str(location='json', required=True, validate=validate.Regexp('/[a-z0-9_/-]*')),
}

CONTAINER: dict[str, Field] = {
    'cpu_guarantee': Float(location='json', validate=validate.Range(min=0.0)),
    'cpu_limit': Float(location='json', validate=validate.Range(min=0.0)),
    'dom0': Str(location='json', allow_none=True),
    'memory_guarantee': SIZE_FIELD(),
    'memory_limit': SIZE_FIELD(),
    'hugetlb_limit': SIZE_FIELD(),
    'net_guarantee': SIZE_FIELD(),
    'net_limit': SIZE_FIELD(),
    'io_limit': SIZE_FIELD(),
    'generation': Int(location='json', validate=validate.Range(min=1, max=3)),
    'extra_properties': Dict(location='json'),
    'secrets': Dict(location='json'),
    'bootstrap_cmd': Str(location='json'),
    'project_id': Str(location='json'),
    'managing_project_id': Str(location='json'),
}

CONTAINER_CREATE: dict[str, Field] = {
    **CONTAINER,
    'project': Str(location='json', required=True),
    'cluster': Str(location='json', required=True),
    'geo': Str(location='json', required=True),
    'restore': Bool(location='json', allow_none=True),
    'volumes': Nested(VOLUME_CREATE, location='json', many=True, requered=True),
}


@APP.after_request
def log_request(response):
    """
    Request result logger
    """
    logger = logging.getLogger('flask.app')
    logger.info('', extra={'response': response})
    return response


@APP.after_request
def db_context_clean(response):
    """
    Context cleanup
    """
    if 300 > response.status_code >= 200:
        commit()
    clean_context()
    return response


@APP.route('/ping')
def ping():
    """
    Handler to monitor that daemon is alive
    """
    if os.path.exists('/tmp/.mdb-dbm-close'):
        abort(405, message=jsonify({'status': 'closed'}))

    return jsonify({'status': 'alive'})


@APP.route('/api/version')
def version():
    """
    Returns current api version
    """
    return jsonify({'version': 2})


@APP.route('/')
@APP.route('/containers')
@check_auth(return_path='/containers')
def containers():
    """
    Containers list view
    """
    return render_template('containers.html')


@APP.route('/volume_backups')
@check_auth(return_path='/volume_backups')
def volume_backups():
    """
    Volume backups list view
    """
    return render_template('volume_backups.html')


@APP.route('/volumes')
@check_auth(return_path='/volumes')
def volumes():
    """
    Volume list view
    """
    return render_template('volumes.html')


@APP.route('/transfers')
@check_auth(return_path='/transfers')
def transfers():
    """
    Transfer list view
    """
    return render_template('transfers.html')


@APP.route('/api/v2/transfers/', methods=['GET'])
@check_auth()
@PARSER.use_kwargs(
    {
        'fqdn': Str(location='query'),
    }
)
def get_container_transfer_wrapper(fqdn=None):
    """
    REST get transfer
    """
    if fqdn is None:
        return jsonify(get_transfers())
    return jsonify(get_transfer_by_container(fqdn))


@APP.route('/api/v2/transfers/<id>', methods=['GET'])
@check_auth()
# pylint: disable=invalid-name,redefined-builtin
def get_transfer_wrapper(id):
    """
    REST get transfer
    """
    return jsonify(get_transfer(id))


@APP.route('/api/v2/transfers/<id>/finish', methods=['POST'])
@check_auth()
# pylint: disable=invalid-name,redefined-builtin
def finish_transfer_wrapper(id):
    """
    Finish transfer
    """
    return jsonify(finish_transfer(id))


@APP.route('/api/v2/transfers/<id>/cancel', methods=['POST'])
@check_auth()
# pylint: disable=invalid-name,redefined-builtin
def cancel_transfer_wrapper(id):
    """
    Cancel transfer
    """
    return jsonify(cancel_transfer(id))


@APP.route('/api/v2/containers/<fqdn>')
@check_auth()
def get_porto_info(fqdn):
    """
    Get container
    """
    return jsonify(get_container(fqdn))


@APP.route('/api/v2/containers/')
@check_auth()
@PARSER.use_kwargs(
    {
        'query': Str(location='query', validate=search_query_validator),
    }
)
def get_containers_info(query=''):
    """
    REST list containers with filter
    """
    return jsonify(get_containers(query))


@APP.route('/api/v2/containers/<fqdn>', methods=['POST'])
@check_auth()
@PARSER.use_kwargs(CONTAINER)
def update_container_wrapper(fqdn, **options):
    """
    REST change container
    """
    return jsonify(update_container(fqdn, options))


@APP.route('/api/v2/containers/<fqdn>', methods=['PUT'])
@check_auth()
@PARSER.use_kwargs(CONTAINER_CREATE)
def create_container(fqdn, project, cluster, **options):
    """
    REST create container
    """
    return jsonify(
        launch_container(
            project,
            cluster,
            {
                **options,
                'fqdn': fqdn,
            },
        )
    )


@APP.route('/api/v2/containers/<fqdn>', methods=['DELETE'])
@check_auth()
@PARSER.use_kwargs(
    {
        'save_paths': Str(),
    }
)
def delete_container(fqdn, save_paths=None):
    """
    Method for marking container to be deleted

    Not REST since save_paths contain /
    """
    return jsonify(destroy_container(fqdn, save_paths.split(',') if save_paths else None))


@APP.route('/api/v2/dom0/delete-report/<fqdn>', methods=['POST'])
@PARSER.use_kwargs(
    {
        'token': Str(location='json', required=True),
    }
)
def dom0_delete_container(fqdn, token):
    """
    Method for finishing container deletion

    It doesn't check auth in normal way since request should contain
    delete token.
    """
    return jsonify(drop_container(fqdn, token))


@APP.route('/api/v2/dom0/volume-backup-delete-report/<dom0>', methods=['POST'])
@PARSER.use_kwargs(
    {
        'path': Str(location='query', required=True, validate=validate.Regexp('/[a-z0-9_/-]*')),
        'token': Str(location='json', required=True),
    }
)
def dom0_drop_volume_backup(dom0, path, token):
    """
    Method for finishing volume backup deletion

    It doesn't check auth in normal way since request should contain
    delete token.
    """
    return jsonify(drop_volume_backup(dom0, path, token))


# We don't use RESTful URL here because
# query string may have symbol '/' in paths
@APP.route('/api/v2/volumes/')
@check_auth()
@PARSER.use_kwargs(
    {
        'query': Str(location='query', validate=search_query_validator),
    }
)
def get_volumes_info(query=''):
    """
    REST list volumes
    """
    return jsonify(get_volumes(query))


@APP.route('/api/v2/volumes/<fqdn>', methods=['POST'])
@check_auth()
@PARSER.use_kwargs(
    {
        **VOLUME,
        'path': Str(location='query', required=True, validate=validate.Regexp('/[a-z0-9_/-]*')),
        'init_deploy': Bool(location='query', missing=True),
    }
)
def update_volume_wrapper(fqdn, path, init_deploy, **options):
    """
    REST change volume
    """
    return jsonify(update_volume(fqdn, path, options, init_deploy))


@APP.route('/api/v2/volumes/<fqdn>', methods=['GET'])
@check_auth()
def get_volume_wrapper(fqdn):
    """
    REST get volumes
    """
    return jsonify(get_container_volumes(fqdn))


@APP.route('/api/v2/volume_backups/')
@check_auth()
@PARSER.use_kwargs(
    {
        'query': Str(location='query', validate=search_query_validator),
    }
)
def get_volume_backups_info(query=''):
    """
    REST list volume backups
    """
    return jsonify(get_volume_backups(query))


@APP.route('/api/v2/volume_backups/<dom0>/<fqdn>', methods=['DELETE'])
@check_auth()
@PARSER.use_kwargs(
    {
        'path': Str(location='query', required=True, validate=validate.Regexp('/[a-z0-9_/-]*')),
    }
)
def delete_volume_backup_wrapper(dom0, fqdn, path):
    """
    REST delete volume backup
    """
    return jsonify(delete_volume_backup(dom0, fqdn, path))


@APP.route('/api/v2/volume_backups_with_token/<dom0>/<fqdn>', methods=['DELETE'])
@PARSER.use_kwargs(
    {
        'path': Str(location='query', required=True, validate=validate.Regexp('/[a-z0-9_/-]*')),
        'token': Str(location='json', required=True),
    }
)
def delete_volume_backup_with_token_wrapper(dom0, fqdn, path, token):
    """
    REST delete volume backup

    It doesn't check auth in normal way since request should contain
    delete token.
    """
    return jsonify(delete_volume_backup(dom0, fqdn, path, token))


@APP.route('/api/v2/pillar/<fqdn>', methods=['GET'])
@check_auth()
def get_ext_pillar_dom0(fqdn):
    """
    dom0 ext-pillar get
    """
    return jsonify(get_pillar(fqdn))


@APP.route('/api/v2/dom0/<fqdn>', methods=['GET'])
@check_auth()
def get_dom0(fqdn):
    """
    REST dom0 get
    """
    return jsonify(get_dom0_host(fqdn))


@APP.route('/api/v2/dom0/<fqdn>', methods=['POST'])
@check_auth()
@PARSER.use_kwargs(
    {
        'project': Str(location='json', required=True),
        'geo': Str(location='json', required=True),
        'switch': Str(location='json'),
        'cpu_cores': Int(location='json', required=True),
        'memory': Int(location='json', required=True),
        'ssd_space': Int(location='json', required=True),
        'sata_space': Int(location='json', required=True),
        'max_io': Int(location='json', required=True),
        'net_speed': Int(location='json', required=True),
        'heartbeat': Bool(location='json', required=True),
        'generation': Int(location='json', validate=validate.Range(min=generations.MIN, max=generations.MAX)),
        'disks': Nested(
            {
                'id': Str(location='json', required=True),
                'max_space_limit': Int(location='json', required=True),
                'has_data': Bool(location='json', required=True),
            },
            location='json',
            many=True,
            required=True,
        ),
    }
)
def update_dom0(fqdn, **options):
    """
    REST dom0 update
    """
    return jsonify(update_dom0_host(fqdn, options))


@APP.route('/api/v2/dom0/allow_new_hosts/<fqdn>', methods=['PUT'])
@check_auth()
@PARSER.use_kwargs(
    {
        'allow_new_hosts': Bool(location='json', required=True),
    }
)
def update_dom0_allow_new_hosts(fqdn, allow_new_hosts):
    """
    REST dom0.allow_new_hosts update
    """
    updated = update_allow_new_hosts(fqdn, allow_new_hosts, g.login)
    if updated:
        return jsonify(updated)
    abort(404)
    return None
