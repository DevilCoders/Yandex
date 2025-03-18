#! /usr/local/bin/python

import re
import os
import sys
import struct
import getopt;

'''
http://wiki.wireshark.org/Development/LibpcapFileFormat

typedef struct pcap_hdr_s {
        guint32 magic_number;   /* magic number */
        guint16 version_major;  /* major version number */
        guint16 version_minor;  /* minor version number */
        gint32  thiszone;       /* GMT to local correction */
        guint32 sigfigs;        /* accuracy of timestamps */
        guint32 snaplen;        /* max length of captured packets, in octets */
        guint32 network;        /* data link type */
} pcap_hdr_t;

typedef struct pcaprec_hdr_s {
        guint32 ts_sec;         /* timestamp seconds */
        guint32 ts_usec;        /* timestamp microseconds */
        guint32 incl_len;       /* number of octets of packet saved in file */
        guint32 orig_len;       /* actual length of packet */
} pcaprec_hdr_t;
'''

def toByteStr(s):
    hexDigits = "0123456789ABCDEF";
    res = "";
    for c in s:
        cc = ord(c);
        res += hexDigits[cc >> 4] + hexDigits[cc & 0xF] + " ";
    return res;
        
def usage():
    print >>sys.stderr, ('usage: %s [-F] PCAP_DUMP_PATH' % sys.argv[0])
    print >>sys.stderr, ('PCAP_DUMP_PATH    path to pcap file or "-" for stdin')
    print >>sys.stderr, ('            -F    full mode')
    sys.exit(1)

getRe = re.compile(r"GET (\S+) HTTP/1.[01]");

def process_short(rh_sec, rh_usec, packet_data):
    match = getRe.search(packet_data);

    if match:
        print('%f\t%s' % (rh_sec + rh_usec / 1000000.0, match.group(1)))

fullRe = re.compile(r"((?:GET|POST).*)", re.DOTALL);

def process_full(rh_sec, rh_usec, packet_data):
    # skip HTTP requests not fitting in one packet
    if not packet_data.endswith("\r\n\r\n"):
        return;

    match = fullRe.search(packet_data);

    if match:
        print match.group(1);



PCAP_FILE_HEADER_MAGIC = 0xa1b2c3d4
PCAP_FILE_HEADER_FORMAT = 'IHHiIII'
PCAP_RECORD_HEADER_FORMAT = 'IIII'

try:
    options, args = getopt.getopt(sys.argv[1:], "F");
    options = dict(options);
    if '-F' in options:
        process_data = process_full;
    else:
        process_data = process_short;
except:
    usage();

if len(args) != 1:
    usage();

if args[0] == '-':
    file = sys.stdin
else:
    file = open(args[0], 'rb')

file_header = file.read(struct.calcsize(PCAP_FILE_HEADER_FORMAT))
(fh_magic, fh_ver_major, fh_ver_minor, X, X, fh_snaplen, X) = struct.unpack(PCAP_FILE_HEADER_FORMAT, file_header)

if fh_magic != PCAP_FILE_HEADER_MAGIC:
    raise Exception, 'invalid or unknown file format'
    sys.exit(1)

while True:
    record_header = file.read(struct.calcsize(PCAP_RECORD_HEADER_FORMAT))

    if len(record_header) == 0:
        sys.exit(0)

    if len(record_header) != struct.calcsize(PCAP_RECORD_HEADER_FORMAT):
        raise Exception, 'read error or unexpected end of file'

    (rh_sec, rh_usec, rh_incl_len, rh_orig_len) = struct.unpack(PCAP_RECORD_HEADER_FORMAT, record_header)

    packet_data = file.read(rh_incl_len)

    if len(packet_data) != rh_incl_len:
        raise Exception, 'read error or unexpected end of file'

    process_data(rh_sec, rh_usec, packet_data);
