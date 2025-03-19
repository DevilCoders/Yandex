""" icecream-agent api http routes representation """
import logging
from flask import jsonify, abort
from icecream_agent.models.hw import Dom0
from icecream_agent.constants import DEFAULT_CONFIG, CONFIG_PATH
from icecream_agent.libs.common_helpers import load_config
from icecream_agent.models.storage import StorageVolumeNotFoundError, StorageNotEnoughSpaceError


CONFIG = load_config(DEFAULT_CONFIG, CONFIG_PATH)

CONFIG_PARAM = [CONFIG.pool]

def ping():
    """ HTTP GET /ping """
    return 'Ok'

def resources():
    """ HTTP GET /resources """
    logging.debug('Request for resources')
    machine = Dom0(*CONFIG_PARAM).to_dict()
    logging.debug('Machine resources: %s', machine)
    return jsonify(machine)


def container_volume(name):
    """ HTTP GET /containers/<container_name>/volume """
    logging.debug('Request for container volume info with name: %s', name)
    machine = Dom0(*CONFIG_PARAM)
    volume = None
    volume_name = machine.storage.container_volume_name(name)
    logging.debug('Compiled volume name from container name: %s', volume_name)
    try:
        volume = machine.storage.get_volume(volume_name).to_dict()
        logging.debug('Got volume info: %s', volume)
    except StorageVolumeNotFoundError:
        logging.debug('Didnt find volume for container with name %s', name)
        abort(404)
    return jsonify(volume)


def container_volume_size(name, payload):
    """ HTTP PUT /containers/<container_name>/volume """
    logging.debug('Request for container volume resize, name: %s, new_size: %s',
                  name, payload)
    machine = Dom0(*CONFIG_PARAM)
    volume = None
    volume_name = machine.storage.container_volume_name(name)
    logging.debug('Compiled volume name from container name: %s', volume_name)
    try:
        volume = machine.storage.get_volume(volume_name)
        logging.debug('Got volume info: %s', volume.to_dict())
    except StorageVolumeNotFoundError:
        logging.debug('Didnt find volume for container with name %s', name)
        abort(404)
    try:
        logging.debug('Resizing to %s', payload)
        return volume.resize(int(payload))
    except NotImplementedError:
        logging.error('Got Notimplemented error!')
        abort(400)
    except StorageNotEnoughSpaceError:
        logging.error('Not enough space!')
        abort(406)
