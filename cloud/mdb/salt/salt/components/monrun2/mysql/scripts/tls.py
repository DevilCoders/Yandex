#!/usr/bin/env python3

import argparse
import socket
import ssl
import struct
import subprocess
import sys
from datetime import datetime

import OpenSSL

from mysql_util import die

PAIRS = {}


def get_expire_date(certificate):
    expire_date = datetime.strptime(certificate.get_notAfter().decode('ascii'),
                                    '%Y%m%d%H%M%SZ')
    return expire_date - datetime.now()


def main():
    args = get_args()
    res = {}
    server_cert_exception_strings = []
    for port in args.PAIRS:
        sock = None
        try:
            sock = socket.create_connection((args.host, port))
            certificate = get_certificate_from_socket(sock)
        except Exception as exc:
            server_cert_exception_strings.append(str(exc))
        else:
            res[port] = [
                encode_der_as_pem(certificate),
                get_expire_date(certificate).days,
            ]
        finally:
            if sock:
                sock.close()
        certificate = read_file(args.PAIRS[port])
        certificate = OpenSSL.crypto.load_certificate(
            OpenSSL.crypto.FILETYPE_PEM, certificate)
        res[args.PAIRS[port]] = [
            encode_der_as_pem(certificate),
            get_expire_date(certificate).days,
        ]
        if res.get(port) and res[port][0] != res[args.PAIRS[port]][0]:
            die(2, 'Certificate on {port} and {file} is different'.format(
                port=port, file=args.PAIRS[port]))
    for location in res:
        if res[location][1] <= args.crit:
            die(2, 'certificate {location} expires in {days} days'.format(
                location=location, days=res[location][1]))
        elif res[location][1] <= args.warn:
            die(1, 'certificate {location} expires in {days} days'.format(
                location=location, days=res[location][1]))
    if server_cert_exception_strings:
        exc_str = ",".join(server_cert_exception_strings)
        die(1, 'Something failed while fetching certificate: {}'.format(exc_str))
    die(0, "OK")


def read_file(filename):
    cmd = ['sudo', '/bin/cat', filename]
    stdout = subprocess.check_output(cmd, shell=False)
    return stdout


class StoreDictKeyPair(argparse.Action):
    def __call__(self, parser, namespace, values, option_string=None):
        for kv in values.split(","):
            k, v = kv.split(":")
            PAIRS[k] = v
        setattr(namespace, self.dest, PAIRS)


def get_args():
    parser = argparse.ArgumentParser()
    parser.add_argument('host', help='Either an IP address, hostname')
    parser.add_argument(
        "--pairs",
        dest="PAIRS",
        action=StoreDictKeyPair,
        help='MySQL certificate port:file')
    parser.add_argument(
        '-w', '--warn', type=int, default=60, help='Warning limit (days)')
    parser.add_argument(
        '-c', '--crit', type=int, default=30, help='Critical limit (days)')
    return parser.parse_args()


def get_certificate_from_socket(sock):
    read_greating(sock)
    send_login_request(sock)
    ssl_context = get_ssl_context()
    sock = ssl_context.wrap_socket(sock)
    sock.do_handshake()
    certificate_as_der = sock.getpeercert(binary_form=True)

    x509 = OpenSSL.crypto.load_certificate(OpenSSL.crypto.FILETYPE_ASN1,
                                           certificate_as_der)
    return x509


def encode_der_as_pem(cert):
    return OpenSSL.crypto.dump_certificate(OpenSSL.crypto.FILETYPE_PEM, cert)


def read_greating(sock):
    data = read_n_bytes_from_socket(sock, 4)
    packet_len, = struct.unpack("i", data[:3] + b"\x00")
    read_n_bytes_from_socket(sock, packet_len)


def send_login_request(sock):
    packet = (
        struct.pack("i", 32)[:3] +  # packet len
        struct.pack("b", 1) +       # seq number
        b"\x05\xaa\xbf\x41" +        # client flags
        b"\x00\x00\x00\x01" +        # max packet size
        b"\xff" +                    # charset
        (b"\x00" * 23)               # padding
    )
    sock.sendall(packet)


def get_ssl_context():
    # Return the strongest SSL context available locally
    for proto in ('PROTOCOL_TLSv1_2', 'PROTOCOL_TLSv1', 'PROTOCOL_SSLv23'):
        protocol = getattr(ssl, proto, None)
        if protocol:
            break
    return ssl.SSLContext(protocol)


def read_n_bytes_from_socket(sock, n):
    buf = bytearray(n)
    view = memoryview(buf)
    while n:
        nbytes = sock.recv_into(view, n)
        view = view[nbytes:]  # slicing views is cheap
        n -= nbytes
    return buf


if __name__ == '__main__':
    main()
