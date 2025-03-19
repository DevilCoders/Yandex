"""pingunoque ipset wrapper"""
import socket
import subprocess
from pingunoque import log, config

IPSET_BINARY = subprocess.check_output(["which", "ipset"]).rstrip()

def ban(host, stale_cache=False):
    """Helper for manage ipset list"""
    ips = host.ips(stale_cache)
    _do('add', ips, host.cfg)

def unban(host, stale_cache=False):
    """Helper for manage ipset list"""
    ips = host.ips(stale_cache)
    _do('del', ips, host.cfg)

def _do(operation, ips, cfg):
    for res in ips:
        addr = res[4][0]
        if res[0] == socket.AF_INET:
            ipset_list = cfg.set_v4
        if res[0] == socket.AF_INET6:
            ipset_list = cfg.set_v6
        log.debug("ipset %s %s", operation, addr)
        proc = subprocess.Popen(
            [IPSET_BINARY, operation, ipset_list, addr],
            stdout=subprocess.PIPE,
            stderr=subprocess.STDOUT,
        )
        out, _ = proc.communicate()
        if out:
            for line in out.splitlines():
                log.debug('%r', line)

def flush_all():
    """flush all ip sets"""
    cfg = config.load_config()
    for set_name in (cfg.get("set_v4", "down4"), cfg.get("set_v6", "down6")):
        log.info("Flushing ipset %s", set_name)
        proc = subprocess.Popen([IPSET_BINARY, "flush", set_name])
        out, _ = proc.communicate()
        if out:
            for line in out.splitlines():
                log.debug('%r', line)

def clean(ips, cfg):
    """Remove stale ips from ipset"""
    _do('del', ips, cfg)
