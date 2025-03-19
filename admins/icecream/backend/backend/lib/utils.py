"""Handy utilites for icecream backend"""
import os
from types import SimpleNamespace as ns
import logging
import yaml
import dotmap
from lib import exceptions as e


LOG = logging.getLogger()


def convert_mem(mem):
    '''convert mem to bytes'''
    try:
        mem = str(mem).lower().strip()
        if mem.endswith("tb"):
            memory = (int(mem[:-2]) * 1024 * 1024 * 1024 * 1024)
        elif mem.endswith("gb"):
            memory = (int(mem[:-2]) * 1024 * 1024 * 1024)
        elif mem.endswith("mb"):
            memory = (int(mem[:-2]) * 1024 * 1024)
        elif mem.endswith("kb"):
            memory = (int(mem[:-2]) * 1024)
        else:
            memory = int(mem)
    except Exception as exc:  # pylint: disable=broad-except
        LOG.error("Bad value about memory: %s - %s", mem, exc)
        memory = 0

    return memory


def load_config():
    """load app configs"""
    config_file = "/etc/yandex/icecream/backend/config.yaml"
    if not os.path.exists(config_file):
        config_file = "/etc/yandex/icecream/backend/config-default.yaml"
    if not getattr(load_config, "config", None):
        with open(config_file) as conf:
            load_config.config = dotmap.DotMap(yaml.load(conf))
        hosts = load_config.config.mongo.hosts or ["localhost"]
        port = load_config.config.mongo.port
        for idx, host in enumerate(hosts):
            if ":" not in host and port:
                host = ":".join((host, str(port)))
                load_config.config.mongo.hosts[idx] = host
        load_config.config.mongo.hosts = ",".join(hosts)

    return load_config.config


def compute_cpuset(amount, old_cpu_set, cpu_avail_list):
    """Compute new cpu_list for container"""
    res = ns(old_set=old_cpu_set, amount=amount)
    if amount > 0:
        res.new_set = set(cpu_avail_list[0:amount]) | res.old_set
        res.diff_set = res.new_set - res.old_set
    elif amount < 0:
        res.new_set = set(list(old_cpu_set)[-amount:])
        res.diff_set = res.old_set - res.new_set

    res.new_list = list(res.new_set)
    if len(res.new_set) == 1:
        res.new_list += res.new_list
    res.config_entry = ",".join(str(int(x)) for x in res.new_list)

    return res

def check_resource(diff, avail, error='Insufficient resources'):
    """Check resource bondaries"""
    if diff > 0 and diff > avail:
        raise e.IceResizeError(
            status=406,
            detail='{}: want {} > have {}'.format(
                error, diff, avail,
            )
        )

def name_from_fqdn(fqdn, kind="short"):
    """make short name from fqdn"""
    short_name = fqdn.replace(".yandex.net", "")
    if kind == "lxd":
        short_name = short_name.replace(".", "--")
    return short_name
