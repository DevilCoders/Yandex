#!/skynet/python/bin/python
"""
    Resolve hosts via specified protocols with retry
"""

import socket
import threading
from collections import defaultdict

from gaux.aux_colortext import red_text

from core.db import CURDB


def resolve_addrs(name, IPv=4):
    assert (IPv in [4, 6])
    sock_family = socket.AF_INET if IPv == 4 else socket.AF_INET6
    try:
        saved = None
        for i in range(2):
            try:
                resolved = socket.getaddrinfo(name, None, sock_family)
                ips = set(ip[4][0] for ip in resolved)
                if len(ips) > 1:
                    print red_text("Multiple addrs for host %s: %s" % (name, ips))
                return True, list(ips)[0]
            except Exception as e:
                saved = e
        raise saved
    except EnvironmentError as e:
        return False, str(e)


class ResolverRoutine(threading.Thread):
    def __init__(self, hosts, family):
        threading.Thread.__init__(self)
        self.hosts = hosts
        assert (family in [4, 6])
        self.family = family
        self.ips = []
        self.unresolved = []

    def run(self):
        for host in self.hosts:
            ip = resolve_addrs(host, self.family)
            self.ips.append(ip)


resolved_hosts = defaultdict(lambda: dict())


def resolve_hosts(hosts, families, use_threads=True, fail_unresolved=True):
    """
        Resolve list of hosts, using standard getaddrsinfo function. Can use threads to resolve in parallel. Cache
        results, so, next calls will return the same result. When found unresolved hosts, exeception raised if fail_unresoved
        is specified

        :type hosts: list[str]
        :type families: list[int]
        :type use_threads: bool
        :type fail_unresolved: bool

        :param hosts: list of hosts to resolve
        :param families: list of protocols to resolve. Must be one of [4], [6], [4, 6], [6, 4].
        :param use_threads: if param is True, run in parallel, otherwise all hosts are resolved consequentially
        :param fail_unresolved: if param is True, raise exception with messages on on unresolved machines
        :return dict(str to str): dict (hostname -> address) for all hosts. For hosts, which can not be resolved address is None
    """

    global resolved_hosts

    # resolve all still non-resolved hosts for all requested families
    for family in families:
        to_resolve = list(set(filter(lambda x: x not in resolved_hosts[family], hosts)))

        if use_threads:
            nthreads = 10
            workers = []
            for i in range(nthreads):
                fst = (i * len(to_resolve)) / nthreads
                lst = ((i + 1) * len(to_resolve)) / nthreads
                workers.append(ResolverRoutine(to_resolve[fst:lst], family))

            for worker in workers:
                worker.start()

            unresolved = []
            for worker in workers:
                worker.join()
                unresolved.extend(worker.unresolved)
                for i in range(len(worker.hosts)):
                    resolved_hosts[family][worker.hosts[i]] = worker.ips[i]
        else:
            for host in to_resolve:
                resolved_hosts[family][host] = resolve_addrs(host, family)

    # create result
    result = {}
    failmsg = []
    for host in hosts:
        hostfailmsg = []
        found = False
        for family in families:
            success, errmsg_or_value = resolved_hosts[family][host]
            if success:
                result[host] = errmsg_or_value
                found = True
                break
            else:
                result[host] = None
                hostfailmsg.append(
                    'Could not resolve "%s" using families <%s>, reason:\n\t%s' % (host, families, errmsg_or_value))

        if not found:
            failmsg.extend(hostfailmsg)

    if failmsg and fail_unresolved:
        raise RuntimeError('\n'.join(failmsg))

    return result


def resolve_host(hostname, families, use_curdb=True, fail_not_found_in_curdb=False, fail_unresolved=True):
    """
        Resolve specified host.

        :type hostname: str
        :type families: list[int]
        :type use_curdb: bool
        :type fail_not_found_in_curdb: bool
        :type fail_unresolved: bool

        :param hostname: fqdn of host to resolve
        :param families: list of protocols to resolve. Must be one of [4], [6], [4, 6], [6, 4].
        :param use_curdb: if param is True, get cached data from CURDB rather than get address by getaddrsinfo
        :param fail_not_found_in_curdb: if param is True, raise exception, if curdb do not have addrs for specified host
        :param fail_unresolved: if param is True, raise exception, when can not resolve host

        :return (str): host ip address
    """

    if use_curdb:
        family_map = {4: 'ipv4addr', 6: 'ipv6addr'}
        if CURDB.hosts.has_host(hostname):
            host = CURDB.hosts.get_host_by_name(hostname)

            for family in families:
                result_addr = getattr(host, family_map[family])
                if result_addr != 'unknown':
                    return result_addr
    if fail_not_found_in_curdb:
        raise Exception(
            "Could not resolve host %s using families <%s>" % (hostname, ",".join(map(lambda x: str(x), families))))

    return resolve_hosts([hostname], families, use_threads=False, fail_unresolved=fail_unresolved).values()[0]
