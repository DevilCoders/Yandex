#!/usr/bin/env python3

import argparse
import logging
import subprocess
import sys
from datetime import datetime
import socket

import OpenSSL

PAIRS = {}


def die(code: int, comment: str):
    print(f'{code};{comment.strip()}')
    sys.exit(0)


def ok():
    die(0, 'OK')


def warn(comment: str):
    die(1, comment)


def crit(comment: str):
    die(2, comment)


def get_expire_date(certificate):
    expire_date = datetime.strptime(certificate.get_notAfter().decode('ascii'),
                                    '%Y%m%d%H%M%SZ')
    return expire_date - datetime.now()


def get_server_certificate(addr):
    """Retrieve the certificate from the server at the specified address,
    and return it as a PEM-encoded string."""
    (host, port) = addr
    cmd1 = ['openssl', 's_client', '-showcerts', '-servername', host, '-connect', '{}:{}'.format(host, port)]
    proc1 = subprocess.Popen(cmd1, stdout=subprocess.PIPE, stderr=subprocess.PIPE)
    cmd2 = ['openssl', 'x509', '-outform', 'PEM']
    proc2 = subprocess.Popen(cmd2, stdin=proc1.stdout, stdout=subprocess.PIPE, stderr=subprocess.PIPE)
    out, err = proc2.communicate()
    if err:
        cmd = ' '.join(cmd1) + ' | ' + ' '.join(cmd2)
        raise RuntimeError(f'"{cmd}" failed')
    return out


def main():
    logging.disable(logging.CRITICAL)

    args = get_args()

    try:
        ping_zk()
    except Exception as e:
        warn(f'Unable to get ZooKeeper status: {e!r}')

    res = {}
    server_cert_exception_strings = []
    for port in args.PAIRS:
        try:
            certificate = get_server_certificate((args.host, port))
        except Exception as e:
            server_cert_exception_strings.append(str(e))
        else:
            try:
                certificate = load_certificate(certificate)
                res[port] = [
                    encode_der_as_pem(certificate),
                    get_expire_date(certificate).days,
                ]
            except Exception as e:
                warn(f'Cannot load certificate from port {port}: {e}')

        try:
            certificate = read_file(args.PAIRS[port])
            certificate = load_certificate(certificate)
            res[args.PAIRS[port]] = [
                encode_der_as_pem(certificate),
                get_expire_date(certificate).days,
            ]
        except Exception as e:
            crit(f'Cannot load certificate from file {args.PAIRS[port]}: {e}')

        if res.get(port) and res[port][0] != res[args.PAIRS[port]][0]:
            crit(f'Certificate on {port} and {args.PAIRS[port]} is different')

    for location in res:
        exp_days = res[location][1]
        if exp_days <= args.crit:
            crit(f'certificate {location} expires in {exp_days} days')
        elif exp_days <= args.warn:
            warn(f'certificate {location} expires in {exp_days} days')

    if server_cert_exception_strings:
        exc_str = ','.join(server_cert_exception_strings)
        crit(f'Failed to get certificate: {exc_str}')

    ok()


def read_file(filename):
    cmd = ['sudo', '/bin/cat', filename]
    stdout = subprocess.check_output(cmd, shell=False)
    return stdout


class StoreDictKeyPair(argparse.Action):
    def __call__(self, parser, namespace, values, option_string=None):
        for kv in values.split(','):
            k, v = kv.split(':')
            PAIRS[k] = v
        setattr(namespace, self.dest, PAIRS)


def get_args():
    parser = argparse.ArgumentParser()
    parser.add_argument('host', help='Either an IP address, hostname')
    parser.add_argument(
        '--pairs',
        dest='PAIRS',
        action=StoreDictKeyPair,
        help='certificate port:file')
    parser.add_argument(
        '-w', '--warn', type=int, default=40, help='Warning limit (days)')
    parser.add_argument(
        '-c', '--crit', type=int, default=30, help='Critical limit (days)')
    return parser.parse_args()


def load_certificate(certificate):
    x509 = OpenSSL.crypto.load_certificate(OpenSSL.crypto.FILETYPE_PEM,
                                           certificate)
    return x509


def encode_der_as_pem(cert):
    return OpenSSL.crypto.dump_certificate(OpenSSL.crypto.FILETYPE_PEM, cert)


def ping_zk():
    with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as s:
        s.settimeout(3)
        s.connect(("localhost", 2181))

        s.sendall(b'mntr')
        return s.makefile().read(-1)


if __name__ == '__main__':
    main()
