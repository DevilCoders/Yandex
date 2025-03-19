#!/usr/bin/env python

import argparse
import socket
from json import loads

import paramiko
from tenacity import retry, stop_after_attempt, wait_fixed

parser = argparse.ArgumentParser()
parser.add_argument('-r', '--retries', type=int, default=2, help='Retry count before segments avalible by ssh')
parser.add_argument('-w', '--wait', type=int, default=5, help='Time to wait after each retry')
parser.add_argument('-s', '--ssh', action='store_true', help='Run ssh check')
args = parser.parse_args()


def get_conn(connect_address):
    """
    Get ssh and sftp conn to address
    """
    ssh = paramiko.SSHClient()
    ssh_pkey = paramiko.RSAKey.from_private_key_file(filename='/home/gpadmin/.ssh/id_rsa')
    ssh.load_system_host_keys()
    ssh.set_missing_host_key_policy(paramiko.AutoAddPolicy())
    username = 'gpadmin'
    ssh.connect(connect_address, username=username, pkey=ssh_pkey, timeout=5, banner_timeout=30, look_for_keys=False)
    session = ssh.get_transport().open_session()
    paramiko.agent.AgentRequestHandler(session)
    session.get_pty()
    session.exec_command('tty')  # noqa
    return ssh


def check_ssh(host):
    """
    Wait for host available through ssh
    """
    conn = get_conn(host)
    _, stdout, stderr = conn.exec_command('/usr/local/yandex/gp_wait_cluster.py -r 1 -w 1')
    assert stdout.channel.recv_exit_status() == 0, "check tcp connection {} failed, stderr: {}".format(
        host, stderr.read())


def check_tcp_conn(host, port=22):
    """
    Wait for host:port to become available
    """
    sock = None
    sock = socket.create_connection((host, port), timeout=0.1)
    if sock:
        sock.close()
        return True


def check(host):
    if check_tcp_conn(host) and args.ssh:
        check_ssh(host)


@retry(stop=stop_after_attempt(args.retries), wait=wait_fixed(args.wait), reraise=True)
def main():
    with open('/etc/dbaas.conf', 'r') as f:
        dbaas_conf = loads(f.read())
        cluster_hosts = dbaas_conf['cluster_hosts']
    for host in cluster_hosts:
        check(host)


if __name__ == '__main__':
    main()
