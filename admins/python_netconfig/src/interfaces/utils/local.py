# based on http://stackoverflow.com/a/20730442/4317857

from socket import AF_INET, AF_INET6, inet_ntop
from ctypes import (
    Structure,
    Union,
    POINTER,
    pointer,
    cast,
    c_ushort,
    c_byte,
    c_void_p,
    c_char_p,
    c_uint,
    c_uint16,
    c_uint32,
)
import ctypes.util
import ctypes
import os
from collections import defaultdict

import interfaces


__all__ = ['LocalInterface', 'get_local_interfaces', 'local_address']


GLOBAL_SCOPE_ID = 0L
LOCAL_INTERFACES = None


class struct_sockaddr(Structure):
    _fields_ = [
        ('sa_family', c_ushort),
        ('sa_data', c_byte * 14),
    ]


class struct_sockaddr_in(Structure):
    _fields_ = [('sin_family', c_ushort), ('sin_port', c_uint16), ('sin_addr', c_byte * 4)]


class struct_sockaddr_in6(Structure):
    _fields_ = [
        ('sin6_family', c_ushort),
        ('sin6_port', c_uint16),
        ('sin6_flowinfo', c_uint32),
        ('sin6_addr', c_byte * 16),
        ('sin6_scope_id', c_uint32),
    ]


class union_ifa_ifu(Union):
    _fields_ = [
        ('ifu_broadaddr', POINTER(struct_sockaddr)),
        ('ifu_dstaddr', POINTER(struct_sockaddr)),
    ]


class struct_ifaddrs(Structure):
    pass


struct_ifaddrs._fields_ = [
    ('ifa_next', POINTER(struct_ifaddrs)),
    ('ifa_name', c_char_p),
    ('ifa_flags', c_uint),
    ('ifa_addr', POINTER(struct_sockaddr)),
    ('ifa_netmask', POINTER(struct_sockaddr)),
    ('ifa_ifu', union_ifa_ifu),
    ('ifa_data', c_void_p),
]

libc = ctypes.CDLL(ctypes.util.find_library('c'))


def ifap_iter(ifap):
    ifa = ifap.contents
    while True:
        yield ifa
        if not ifa.ifa_next:
            break
        ifa = ifa.ifa_next.contents


def getfamaddr(sa):
    family = sa.sa_family
    addr = None
    scope = None
    if family == AF_INET:
        sa = cast(pointer(sa), POINTER(struct_sockaddr_in)).contents
        addr = inet_ntop(family, sa.sin_addr)
        scope = None
    elif family == AF_INET6:
        sa = cast(pointer(sa), POINTER(struct_sockaddr_in6)).contents
        addr = inet_ntop(family, sa.sin6_addr)
        scope = sa.sin6_scope_id
    return family, addr, scope


class LocalInterface(object):
    def __init__(self, name):
        self.name = name
        self.index = libc.if_nametoindex(name)
        self.addresses = defaultdict(list)

    def __str__(self):
        return "%s [index=%d, IPv4=%s, IPv6=%s]" % (
            self.name,
            self.index,
            repr(self.addresses.get(AF_INET)),
            repr(self.addresses.get(AF_INET6)),
        )


def get_local_interfaces():
    global LOCAL_INTERFACES

    def _get():
        ifap = POINTER(struct_ifaddrs)()
        result = libc.getifaddrs(pointer(ifap))
        if result != 0:
            return {}
        del result
        try:
            retval = {}
            for ifa in ifap_iter(ifap):
                name = ifa.ifa_name
                i = retval.get(name)
                if not i:
                    i = retval[name] = LocalInterface(name)
                try:
                    family, addr, scope = getfamaddr(ifa.ifa_addr.contents)
                except ValueError:
                    family, addr, scope = None, None, None
                if addr and family != AF_INET6 or scope == GLOBAL_SCOPE_ID:
                    i.addresses[family].append(addr)
            return retval
        except:
            return {}
        finally:
            libc.freeifaddrs(ifap)

    if LOCAL_INTERFACES is None:
        LOCAL_INTERFACES = _get()
    return LOCAL_INTERFACES


def local_address(address):
    interfaces = get_local_interfaces()
    for iface in interfaces.values():
        for family, addrs in iface.addresses.items():
            if address in addrs:
                return iface.name
    return None


def get_first_uplink_name():
    if interfaces.DEBUG:
        print ("Detecting first alive uplink")
    links = [link for link in os.listdir('/sys/class/net') if link.count('eth')]
    for link in links:
        try:
            carrier = open('/sys/class/net/%s/carrier' % link).read()
            operstate = open('/sys/class/net/%s/operstate' % link).read()
            if interfaces.DEBUG:
                print link, carrier, operstate
            if carrier.lower().count('1') and operstate.lower().count('up'):
                print ("Returning " + link)
                return link
        except IOError as err:
            if interfaces.DEBUG:
                print err
    print ("None found")
    return None
