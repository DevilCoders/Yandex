import dns.resolver

def resolveFqdn(fqdn):
    # The default resolver at least on macOS caches fqdns for too long, request the ip addresses from DNS
    # servers directly.
    try:
        addrs = [r.address for r in dns.resolver.query(fqdn, "AAAA")]
        if len(addrs) == 0:
            return None
        else:
            return addrs[0]
    except dns.resolver.NXDOMAIN:
        return None
