#!/usr/bin/env python3
from collections import defaultdict
from dns import resolver, reversename
import socket
from yc_monitoring import JugglerPassiveCheck


def check_dns(check: JugglerPassiveCheck):
    host_fqdn = socket.getfqdn()
    info = defaultdict(set)
    addrs = set(map(str, (el[4][0] for el in socket.getaddrinfo(host_fqdn, 0, socket.AF_INET6) if el[4][0] != "::1")))
    for address in addrs:
        try:
            reverse_record = reversename.from_address(address)
            resolved_host_name = str(resolver.query(reverse_record, "PTR")[0])
            addr6 = str(resolver.query(resolved_host_name, "AAAA")[0])
            info[resolved_host_name].add(addr6)
        except Exception as ex:
            if isinstance(ex, resolver.NXDOMAIN):
                check.crit("Can't resolve name or address: {0}".format(address))
                continue
            raise

    if len(info.keys()) > 1:
        check.crit("Host has more than one address resolved in different fqdn")
        return

    try:
        resolved_host_name, ip_addresses = info.popitem()
    except KeyError:
        check.crit("No any address found on interfaces")
        return
    if len(ip_addresses) > 1:
        check.crit("FQDN '{}' resolved into few records: {}".format(resolved_host_name, ", ".join(ip_addresses)))
    if host_fqdn not in resolved_host_name:
        check.crit("Hostname and resolved AAAA record are different")


def main():
    check = JugglerPassiveCheck("dns")
    try:
        check_dns(check)
    except Exception as ex:
        check.crit("During check exception raised: ({}): {}".format(ex.__class__.__name__, ex))
    print(check)


if __name__ == "__main__":
    main()
