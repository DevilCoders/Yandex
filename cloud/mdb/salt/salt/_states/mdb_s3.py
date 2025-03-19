# -*- coding: utf-8 -*-
"""
States for managing S3 objects in MDB.
"""

import logging

log = logging.getLogger(__name__)

__salt__ = {}
__opts__ = {}


def __virtual__():
    if 'mdb_s3.client' not in __salt__:
        return False, 'Unable to load mdb_s3 module.'
    return True


def object_present(name):
    """
    Ensure that the specified object exists in S3.
    """
    try:
        s3_client = __salt__['mdb_s3.client']()

        if __salt__['mdb_s3.object_exists'](s3_client, name):
            return {'name': name, 'changes': {}, 'result': True, 'comment': 'Object "{0}" already exists'.format(name)}

        changes = {'created': name}

        if __opts__['test']:
            return {
                'name': name,
                'changes': changes,
                'result': None,
                'comment': 'Object "{0}" will be created'.format(name),
            }

        __salt__['mdb_s3.create_object'](s3_client, name)
        return {'name': name, 'changes': changes, 'result': True, 'comment': 'Object "{0}" was created'.format(name)}

    except Exception as e:
        log.exception('mdb_s3.object_preset failed:')
        return _error(name, str(e))


def object_absent(name):
    """
    Ensure that the specified object not exists in S3.
    """
    try:
        s3_client = __salt__['mdb_s3.client']()

        if not __salt__['mdb_s3.object_exists'](s3_client, name):
            return {'name': name, 'changes': {}, 'result': True, 'comment': 'Object "{0}" already absent'.format(name)}

        changes = {'removed': name}

        if __opts__['test']:
            return {
                'name': name,
                'changes': changes,
                'result': None,
                'comment': 'Object "{0}" will be removed'.format(name),
            }

        __salt__['mdb_s3.object_absent'](s3_client, name)
        return {'name': name, 'changes': changes, 'result': True, 'comment': 'Object "{0}" was removed'.format(name)}
    except Exception as e:
        log.exception('mdb_s3.object_absent failed:')
        return _error(name, str(e))


def _error(name, comment):
    return {'name': name, 'changes': {}, 'result': False, 'comment': comment}
