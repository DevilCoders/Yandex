import logging
import copy
import os
import time
from typing import Any
import yaml

log = logging.getLogger(__name__)


__virtualname__ = "dynamic_pillar_roots"

__opts__: dict[str, Any] = {}
__grains__: dict[str, Any] = {}


def __virtual__():
    return __virtualname__


def envs():
    """
    Return the file server environments
    """
    return sorted(_envs_config())


cache: dict[str, Any] = {}


def _envs_mtime():
    return cache.get("_envs_config", {}).get("mtime", 0)


def _envs_config():
    start = time.time()
    log.debug("_envs_config start")
    path = __opts__["dynamic_pillar_config"]
    mtime = os.path.getmtime(path)
    if _envs_mtime() < mtime:
        with open(path, "rb") as data:
            config = yaml.load(data, Loader=yaml.CSafeLoader)
        cache["_envs_config"] = {
            "mtime": mtime,
            "value": config,
        }
    log.debug("_envs_config took %s seconds", time.time() - start)
    return cache["_envs_config"]["value"]


def ext_pillar(minion_id, pillar, *args, **kwargs):
    import salt.utils.dictupdate
    from salt.pillar import Pillar

    opts = copy.deepcopy(__opts__)
    ret = {}
    log.debug("dynamic_roots args: %s kwargs: %s", args, kwargs)
    pillarenv = opts.get("pillarenv")
    saltenv = opts.get("saltenv")
    log.debug("dynamic_roots saltenv: %s pillarenv %s", saltenv, pillarenv)

    envs_config = _envs_config()
    if pillarenv in envs_config:
        opts["pillar_roots"] = {pillarenv: envs_config[pillarenv]}
        merge_strategy = opts.get("pillar_source_merging_strategy", "smart")
        merge_lists = opts.get("pillar_merge_lists", False)

        log.debug("pillar_roots %s", opts["pillar_roots"])
        local_pillar = Pillar(
            opts=opts,
            grains=__grains__,
            minion_id=minion_id,
            saltenv=saltenv,
            pillarenv=pillarenv,
        )
        ret = salt.utils.dictupdate.merge(
            ret,
            local_pillar.compile_pillar(ext=False),
            strategy=merge_strategy,
            merge_lists=merge_lists,
        )
    return ret
