#!/usr/bin/python

import logging
import sys

__virtualname__ = 'porto'
_VERSION = '0.1.0'

try:
    import porto as api
    import socket
    from six.moves import http_client as httplib

    MODULES_OK = True
except:
    MODULES_OK = False

log = logging.getLogger(__name__)


def __virtual__():
    if __grains__.get('virtual') != 'lxc' and MODULES_OK:
        log.debug('"porto" module loaded, version %s' % _VERSION)
        global conn
        conn = api.Connection()
        conn.connect()
        return __virtualname__
    return False


def __clear_dict(dictionary):
    res = {}
    for k, v in dictionary.items():
        if not k.startswith('__'):
            res[k] = v
    return res


def volumes_list():
    res = {}
    for vol in conn.ListVolumes():
        res[vol.path] = vol.GetProperties()
    return res


def volume_info(path):
    return {path: conn.FindVolume(path).GetProperties()}


def storage_info(storage):
    res = {}
    for vol in conn.ListVolumes():
        if vol.GetProperty('storage') == storage:
            res[vol.path] = vol.GetProperties()
    return res


def create_volume(storage, **properties):
    if any([vol.GetProperty('storage') for vol in conn.ListVolumes() if vol.GetProperty('storage') == storage]):
        return storage_info(storage)
    properties = __clear_dict(properties)
    properties['storage'] = storage
    return volume_info(conn.CreateVolume(path=None, **properties).path)


def tune_volume(path, **properties):
    vol = conn.FindVolume(path)

    properties = __clear_dict(properties)
    if properties == {}:
        return volume_info(vol.path)

    need_update = {}
    for p in properties.keys():
        if properties[p] != vol.GetProperty(p):
            need_update[p] = properties[p]

    if need_update == {}:
        return volume_info(vol.path)
    else:
        vol.Tune(**need_update)
        return volume_info(vol.path)


def link_volume(path, name):
    vol = conn.FindVolume(path)
    container = conn.Find(name)
    try:
        vol.Link(container)
        return True
    except api.exceptions.VolumeAlreadyLinked:
        return None
    except Exception:
        return False


def unlink_volume(path, name):
    vol = conn.FindVolume(path)
    container = conn.Find(name)
    try:
        vol.Unlink(container)
        return True
    except api.exceptions.VolumeNotLinked:
        return None
    except Exception:
        return False


def containers_list():
    res = [i for i in conn.List() if i != '/']
    return res


def container_info(name):
    if name not in conn.List():
        return None
    return {name: conn.Find(name).GetProperties()}


def container_state(name):
    if name not in conn.List():
        return None
    return conn.Find(name).GetData('state')


def create_container(name):
    if name in conn.List():
        return {name: conn.Find(name).GetProperties()}

    container = conn.Create(name)
    return {name: conn.Find(name).GetProperties()}


def destroy_container(name):
    if name not in conn.List():
        return None
    container = conn.Find(name)
    try:
        container.Destroy()
        return True
    except Exception as err:
        log.error(error)
        return False


def tune_container(name, **properties):
    if name not in conn.List():
        return None
    container = conn.Find(name)

    properties = __clear_dict(properties)
    if properties == {}:
        return container_info(name)

    need_update = {}
    for p in properties.keys():
        try:
            if properties[p] != container.GetProperty(p):
                need_update[p] = properties[p]
        except api.exceptions.InvalidProperty:
            log.error('Property %s is not available' % p)

    if need_update == {}:
        return container_info(name)
    else:
        for k, v in need_update.items():
            try:
                container.SetProperty(k, v)
            except api.exceptions.InvalidState as err:
                """
                We return None and don't use custom exception here
                because __salt__ can't use classes, only functions.
                """
                return None
        return container_info(name)


def container_action(name, action):
    if name not in conn.List():
        return False
    container = conn.Find(name)
    if action == 'start':
        container.Start()
    elif action == 'stop':
        container.Stop()
    elif action == 'pause':
        container.Pause()
    elif action == 'resume':
        container.Resume()
    else:
        return False
    return True


if __name__ == '__main__':
    import json

    # set up logging
    log = logging.getLogger(__name__)
    log.setLevel(logging.DEBUG)
    ch = logging.StreamHandler(sys.stderr)
    ch.setLevel(logging.DEBUG)
    log.addHandler(ch)

    method = sys.argv[1]
    args = []
    if sys.argv[2:]:
        args = sys.argv[2:]
    try:
        if args:
            print(json.dumps(globals()[method](*args), indent=4))
        else:
            print(json.dumps(globals()[method](), indent=4))
    except KeyError:
        log.error("method %s is not implemented\n" % method)
    except TypeError as e:
        log.error("unable to execute '%s': %s\n" % (method, str(e)) )
