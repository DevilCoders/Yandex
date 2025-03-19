#!/usr/bin/python
import logging
import sys
import json

_VERSION = '0.4.d'
try:
    import socket
    import re
    from six.moves import http_client as httplib
    import yaml

    MODULES_OK = True
except:
    MODULES_OK = False
log = logging.getLogger(__name__)


def __virtual__():
    if MODULES_OK is True:
        log.debug('"golem" module loaded, version %s' % _VERSION)
        return 'golem'
    return False


def fw(macro, af='any'):
    """
    Resolve golem firewall macro into a list of hosts.
    CLI examples:
    # Resolve both ip4 and ip6 addresses of golem macro _GOLEM_:
    $ salt minionid golem.fw _GOLEM_
    # Resolve ip6 only addresses of _C_MAIL_SETTINGS_:
    $ salt minionid golem.fw _C_MAIL_SETTINGS_ af=ip6
    $ salt minionid golem.fw fdef::1/128
    $ salt minionid golem.fw ya.ru
    """
    # normalize address family names
    if af in ['ip', 'ipv4', 'ip4', 'inet']:
        af = 'ip4'
    elif af in ['ip6', 'ipv6', 'inet6']:
        af = 'ip6'

    return _fw(macro, af)


def _fw(macro, af):
    try:
        return host2ip(macro, af)
    except Exception as e:
        try:
            return _expand_macro(macro, af)
        except:
            log.critical('unable to resolve "%s" to ip/mask, hostname or golem macro' % macro)
            return False

    return False


def host2ip(hostname, af):
    # Hostname can potentially have many addresses.
    addrs = []

    netmask = ''
    # IP address may optionally be supplied with mask.
    host = hostname.split('/', 1)[0]
    # Check netmask if present. Null value maps to 0, illegal strings throw ValueError.
    try:
        int(hostname.split('/', 1)[1])
        netmask = hostname.split('/', 1)[1]
    except IndexError:
        pass
    try:
        if netmask:
            # ip6?
            if ':' in host:
                if af == 'any' or af == 'ip6':
                    return [hostname]
            # ip4.
            else:
                if af == 'any' or af == 'ip4':
                    return [hostname]

        # It may also a hostname or an IP.
        else:
            # this is a list, so we merge it.
            addrs += _resolveip(hostname, af)
    except Exception as e:
        log.debug('string "%s" does not look like an ip, ip/mask or hostname' % hostname)
        raise Exception

    # This casting trickery is required to avoid possible duplicates.
    # 'Set' only has unique values, and 'list' is what salt expects to be returned.
    address_list = list(set(addrs))
    log.debug("hostname '%s', af %s, resolved to %s" % (hostname, af, address_list))
    return address_list


def _expand_macro(macro, af):
    hostnames = []
    addrs = []
    try:
        # Stage 1: instantiate object
        log.debug('Starting new connection to hbf.yandex.net:80')
        conn = httplib.HTTPConnection('hbf.yandex.net', timeout=100)
        conn.request('GET', '/macros/%s' % macro)
        # Retrieve the answer object
        response = conn.getresponse()
        # 400 mean not found.
        if response.status == 400:
            raise Exception('firewall macro not found: %s' % macro)

        # anything else except 200 is also an error.
        if response.status != 200:
            raise Exception('bad status code from host: %i' % response.status)
        # Delimiter: " or ", with newline marking EOF.
        data = response.read()
        hostnames = json.loads(data)
        log.debug('macro %s resolved to: "%s"' % (macro, ','.join(hostnames)))
        conn.close()
        for host in hostnames:
            # Merge two lists
            addrs += host2ip(host, af)

        # This casting trickery is required to avoid possible duplicates.
        # 'Set' only has unique values, and 'list' is what salt expects to be returned.
        return list(set(addrs))
    except Exception as e:
        log.critical('unable to fetch data: %s' % str(e))
        conn.close()
        return False


def _resolveip(host, af='any'):
    ip4 = []
    ip6 = []
    if not socket.getaddrinfo(host, 0):
        log.warn('could not resolve host "%s"' % str(host))
        return []
    for response in socket.getaddrinfo(host, 0):
        family, socktype, proto, canonname, sockaddr = response
        # IP6
        if family == socket.AF_INET6:
            ip6.append(sockaddr[0])
        # IP4.
        elif family == socket.AF_INET:
            ip4.append(sockaddr[0])
    if af == 'any':
        return ip4 + ip6
    elif af == 'ip4':
        return ip4
    elif af == 'ip6':
        return ip6


if __name__ == '__main__':
    import sys
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
        log.exception("unable to execute '%s'" % method)
