"""DNSCacher for pingunoque"""
import time
import socket
from pingunoque import log, ipset

class DNSCacheGAIError(Exception):
    """Exception denote DNS errors"""


class DNSCacheNXDomain(Exception):
    """Exception denote DNS errors"""


class DNSCacher(object):
    """Cache dns answer, on first error return cached name"""
    def __init__(self, name, cfg):

        self.last_update = time.time()
        self.name_not_found = False
        self.dns_errors = 0

        self.name = name
        self.cfg = cfg
        self.__ips = self.dns_query()
        self.cache_ago = time.time() - self.last_update

    def dns_query(self):
        """do dns query"""
        try:
            hai = socket.getaddrinfo(
                self.name, self.cfg.check_port, 0, 0, self.cfg.proto_number
            )
            return hai
        except socket.gaierror as exc:
            self.dns_errors += 1
            log.trace("%s DNSError(%s): %s", self, self.dns_errors, exc)
            raise DNSCacheGAIError(exc)

    def ips(self, force_cache=False):
        """
        Return all known address for this host
        Get address from cache, update cache if need
        """
        self.cache_ago = time.time() - self.last_update
        cache_invalid = self.cache_ago > self.cfg.dns_cache_time
        if not force_cache and cache_invalid:
            try:
                log.trace("%s Update dns cache", self)
                new_ips = self.dns_query()
                missing_ips = set(self.__ips) - set(new_ips)
                if missing_ips:
                    log.trace("%s Clean stale ip %s", self)
                    ipset.clean(missing_ips, self.cfg)
                self.last_update = time.time()
                self.dns_errors = 0
            except DNSCacheGAIError as gai_exc:
                if gai_exc.args[0].errno == -2: # pylint: disable=no-member
                    raise DNSCacheNXDomain(gai_exc.args)
                else:
                    if self.dns_errors > self.cfg.dns_error_limit:
                        raise
        else:
            log.trace("%s Use dns cache", self)
        return self.__ips

    def __str__(self):
        return "DNSCacher({0}, age={1:.3f})".format(repr(self.name), self.cache_ago)
