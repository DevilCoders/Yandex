import sys
from struct import pack, unpack, calcsize
from array import array
from zlib import crc32

def pack_time_ip(t, ip):
    ip4 = map(int, reversed(ip.split('.')))
    if len(ip4) == 4: # IPv4
        return pack('dBBBB', t, *ip4)
    else: # IPv6
        return pack('di', t, crc32(ip))


#unpack_time_ip = lambda s: unpack('dI', s)
def unpack_time_ip(s):
    try:
        return unpack('dI', s)
    except:
        print >>sys.stderr, "Bad unpack, len = ", len(s), repr(s)
        raise


def pack_times_ips(ts, ips):
    return array('d', ts).tostring() + array('I', ips).tostring()


def unpack_times_ips(s):
    n = len(s) / calcsize('dI') * calcsize('d')
    times, ips = array('d'), array('I')
    times.fromstring(s[:n])
    ips.fromstring(s[n:])
    return times, ips
