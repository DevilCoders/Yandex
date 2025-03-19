#!/usr/bin/env python3

import argparse
import ssl
import struct
import subprocess
import sys
from datetime import datetime

import OpenSSL

PAIRS = {}


def die(code=0, comment="OK"):
    if code == 0:
        print("0;OK")
    else:
        print('%d;%s' % (code, comment))
    sys.exit(0)


def get_expire_date(certificate):
    expire_date = datetime.strptime(certificate.get_notAfter().decode('ascii'),
                                    '%Y%m%d%H%M%SZ')
    return expire_date - datetime.now()


def main():
    args = get_args()
    res = {}
    server_cert_exception_strings = []
    for port in args.PAIRS:
        try:
            certificate = ssl.get_server_certificate((args.host, port))
        except Exception as e:
            server_cert_exception_strings.append(str(e))
        else:
            certificate = load_certificate(certificate)
            res[port] = [
                encode_der_as_pem(certificate),
                get_expire_date(certificate).days,
            ]
        certificate = read_file(args.PAIRS[port])
        certificate = load_certificate(certificate)
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
        die(1, 'Failed to get certificate: {}'.format(exc_str))
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
        help='Redis certificate port:file')
    parser.add_argument(
        '-w', '--warn', type=int, default=60, help='Warning limit (days)')
    parser.add_argument(
        '-c', '--crit', type=int, default=30, help='Critical limit (days)')
    return parser.parse_args()


def load_certificate(certificate):
    x509 = OpenSSL.crypto.load_certificate(OpenSSL.crypto.FILETYPE_PEM,
                                           certificate)
    return x509


def encode_der_as_pem(cert):
    return OpenSSL.crypto.dump_certificate(OpenSSL.crypto.FILETYPE_PEM, cert)


if __name__ == '__main__':
    main()
