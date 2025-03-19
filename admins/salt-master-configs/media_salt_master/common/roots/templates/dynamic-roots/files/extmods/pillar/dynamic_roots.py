from __future__ import absolute_import, print_function, unicode_literals

import logging
import copy
import os
import time

import salt.utils.dictupdate
import salt.utils.yaml

from salt.pillar import Pillar


log = logging.getLogger(__name__)


__virtualname__ = 'dynamic_pillar_roots'


def __virtual__():
    return __virtualname__

def envs():
    '''
    Return the file server environments
    '''
    return sorted(_envs_config())


cache = {}


def _envs_mtime():
    return cache.get('_envs_config', {}).get('mtime', 0)


def _envs_config():
    start = time.time()
    log.debug('_envs_config start')
    path = __opts__['dynamic_pillar_config']
    mtime = os.path.getmtime(path)
    if _envs_mtime() < mtime:
        with salt.utils.files.fopen(path, 'rb') as data:
            config = salt.utils.yaml.load(data)
        cache['_envs_config'] = {
            'mtime': mtime,
            'value': config,
        }
    log.debug('_envs_config took %s seconds', time.time() - start)
    return cache['_envs_config']['value']


def ext_pillar(minion_id, pillar, *args, **kwargs):
    opts = copy.deepcopy(__opts__)
    ret = {}
    log.debug('saltenv: %s pillarenv %s', __opts__['saltenv'], __opts__['pillarenv'])
    opts['pillarenv'] = opts.get('pillarenv') or opts.get('saltenv')
    log.debug('saltenv: %s pillarenv %s', opts['saltenv'], opts['pillarenv'])
    env = opts.get('pillarenv')

    envs_config = _envs_config()
    if env in envs_config:
        opts['pillar_roots'] = {env: envs_config[env]}
        merge_strategy = opts.get("pillar_source_merging_strategy", "smart")
        merge_lists = opts.get("pillar_merge_lists", False)

        log.debug('pillar_roots %s', opts['pillar_roots'])
        local_pillar = Pillar(opts, __grains__, minion_id, env)
        ret = salt.utils.dictupdate.merge(
            ret,
            local_pillar.compile_pillar(ext=False),
            strategy=merge_strategy,
            merge_lists=merge_lists
        )
    return ret
