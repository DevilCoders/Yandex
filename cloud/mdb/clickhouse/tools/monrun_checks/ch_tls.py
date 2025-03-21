import click
import socket
import ssl
import subprocess

from datetime import datetime
from typing import List, Optional, Tuple
from OpenSSL.crypto import dump_certificate, load_certificate, FILETYPE_PEM

from cloud.mdb.clickhouse.tools.monrun_checks.clickhouse_client import ClickhouseClient, ClickhousePort
from cloud.mdb.clickhouse.tools.monrun_checks.result import Result

CERTIFICATE_PATH = '/etc/clickhouse-server/ssl/server.crt'


@click.command('tls')
@click.option('-c', '--critical', 'crit', type=int, default=10, help='Critical threshold.')
@click.option('-w', '--warning', 'warn', type=int, default=30, help='Warning threshold.')
@click.option(
    '-p',
    '--ports',
    'ports',
    type=str,
    default=None,
    help='Comma separated list of ports. By default read from ClickHouse config',
)
def tls_command(crit: int, warn: int, ports: Optional[str]) -> Result:
    """
    Checks TLS certificate for expiration and that actual cert from fs used.
    """
    file_certificate, _ = read_cert_file()

    for port in get_ports(ports):
        try:
            certificate, days_to_expire = load_certificate_info(ssl.get_server_certificate((socket.getfqdn(), port)))
        except Exception as e:
            return Result(1, f'Failed to get certificate: {repr(e)}')

        if certificate != file_certificate:
            return Result(2, f'certificate on {port} and {CERTIFICATE_PATH} is different')
        if days_to_expire < crit:
            return Result(2, f'certificate {port} expires in {days_to_expire} days')
        if days_to_expire < warn:
            return Result(1, f'certificate {port} expires in {days_to_expire} days')

    return Result(0, "OK")


def get_ports(ports: Optional[str]) -> List[str]:
    if ports:
        return ports.split(',')
    client = ClickhouseClient()
    return [client.get_port(ClickhousePort.https), client.get_port(ClickhousePort.tcps)]


def read_cert_file() -> Tuple[str, int]:
    cmd = ['sudo', '/bin/cat', CERTIFICATE_PATH]
    stdout = subprocess.check_output(cmd, shell=False)
    return load_certificate_info(stdout)


def load_certificate_info(certificate) -> Tuple[str, int]:
    x509 = load_certificate(FILETYPE_PEM, certificate)
    expire_date = datetime.strptime(x509.get_notAfter().decode('ascii'), '%Y%m%d%H%M%SZ')
    return dump_certificate(FILETYPE_PEM, x509), (expire_date - datetime.now()).days
