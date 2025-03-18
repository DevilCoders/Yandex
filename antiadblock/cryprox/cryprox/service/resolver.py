import socket
import logging

from cachetools import TTLCache
from tornado import gen
from tornado.netutil import Resolver, BlockingResolver
from netaddr import IPAddress


NO_ADDRESSES_EXCEPTION_MESSAGE = "No allowed addresses found"


class BlacklistingResolver(Resolver):
    """
    Wraps a resolver with a list of allowed addresses and hostnames matching these addresses.
    It's used to filter out malicious/unknown addresses from external domains
    """
    # noinspection PyMethodOverriding
    def initialize(self, resolver, blacklisted_networks, request_id=None, service_id=None):
        self.resolver = resolver
        self.blacklisted_networks = blacklisted_networks
        self.request_id = request_id
        self.service_id = service_id

    def close(self):
        self.resolver.close()

    def _is_address_allowed(self, address):
        for network in self.blacklisted_networks:
            if IPAddress(address) in network:
                return False
        return True

    @gen.coroutine
    def resolve(self, host, port, *args, **kwargs):
        resolved_addresses = yield self.resolver.resolve(host, port, *args, **kwargs)

        # Address is actually a tuple with a tuple (AF_FAMILY, (address, port))
        allowed_addresses = [address for address in resolved_addresses if self._is_address_allowed(address[1][0])]

        forbidden_addresses = list(set(resolved_addresses) - set(allowed_addresses))
        if forbidden_addresses:
            logging.error("Non-whitelisted addresses found", forbidden_nets=list(forbidden_addresses), request_id=self.request_id, service_id=self.service_id)

        if not allowed_addresses:
            raise IOError(NO_ADDRESSES_EXCEPTION_MESSAGE)
        raise gen.Return(allowed_addresses)


class CachedResolver(BlockingResolver):
    cache = TTLCache(maxsize=10000, ttl=300)

    def initialize(self, io_loop=None):
        super(CachedResolver, self).initialize(io_loop=io_loop)

    @gen.coroutine
    def resolve(self, host, port, *args, **kwargs):
        address = CachedResolver.cache.get((host, port))
        if address is None:
            # result is a list of (family, address) pairs, where address is a tuple suitable to pass to `socket.connect` (i.e. a ``(host, port)``)
            address = yield super(CachedResolver, self).resolve(host, port, *args, **kwargs)
            # cache only Nat64 responses (cant get ipv4 address from it)
            if socket.AF_INET not in (a[0] for a in address):
                CachedResolver.cache[(host, port)] = address
        raise gen.Return(address)
