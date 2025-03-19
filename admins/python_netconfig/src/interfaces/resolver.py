import interfaces
import socket  # noqa
import dns.resolver
from dns.resolver import NXDOMAIN


class DNSError(Exception):
    pass


def __request(fqdn, qtype):
    try:
        return [item.address for item in dns.resolver.query(qname=fqdn, rdtype=qtype, raise_on_no_answer=False)]
    except NXDOMAIN as e:
        if interfaces.DEBUG:
            print "Resolving %s failed (NXDOMAIN): %s" % (fqdn, e)
    return []


def resolve4_first(fqdn):
    ips = resolve4_all(fqdn)
    if len(ips) > 0:
        return ips[0]
    return None


def resolve6_first(fqdn):
    ips = resolve6_all(fqdn)
    if len(ips) > 0:
        return ips[0]
    return None


def resolve_cname(fqdn):
    return __request(fqdn, qtype='CNAME')


def resolve4_all(fqdn):
    return __request(fqdn, qtype='A')


def resolve6_all(fqdn):
    return __request(fqdn, qtype='AAAA')
