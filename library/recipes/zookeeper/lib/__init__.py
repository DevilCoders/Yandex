import argparse
import os.path
import subprocess
import sys
import tarfile
import tempfile
import time
from kazoo import client as zk_client

from library.python.testing.recipe import set_env
import yatest.common
import yatest.common.network


ZOOKEEPER_DIRECTORY = 'apache-zookeeper-3.6.2-bin'


def log(msg):
    print(msg)
    sys.stdout.flush()


def extract(file):
    tar = tarfile.open(file)
    tar.extractall()
    tar.close()


def write_config(port, tick_time, max_session_timeout):
    log('Creating zoo.cfg...')
    with open('{}/conf/zoo.cfg'.format(ZOOKEEPER_DIRECTORY), 'w') as f:
        f.write('tickTime={}\n'.format(tick_time))
        f.write('maxSessionTimeout={}\n'.format(max_session_timeout))
        f.write('dataDir={}\n'.format(tempfile.mkdtemp(prefix='zookeeper-tmp', dir=yatest.common.ram_drive_path())))
        f.write('maxClientCnxns=0\n')
        f.write('clientPort={}\n'.format(port))
        f.write('admin.enableServer=false\n')
        f.write('4lw.commands.whitelist=*\n')


def run_command(command):
    java_home = os.path.dirname(os.path.dirname(yatest.common.java_path()))
    out_path = yatest.common.output_path()
    process = subprocess.Popen(
        ['{}/bin/zkServer.sh'.format(ZOOKEEPER_DIRECTORY), command, 'zoo.cfg'],
        env={'JAVA_HOME': java_home, 'JVMFLAGS': '-Xmx256m -Djute.maxbuffer=8388608', 'ZOO_LOG_DIR': str(out_path)})
    stdout, stderr = process.communicate()
    return_code = process.wait()
    if return_code:
        log('Process {} failed with return_code {}'.format(process.pid, return_code))
        raise RuntimeError(return_code, process.pid, stdout, stderr)


def wait_for_start(port, timeout=60):
    for attempts in range(timeout):
        try:
            zk = None
            run_command('status')
            zk = zk_client.KazooClient(hosts='127.0.0.1:{}'.format(port))
            zk.start()
            zk.get_children("/")
            return
        except:
            log('status returned False, retrying')
            time.sleep(1)
        finally:
            if zk:
                zk.stop()
    raise RuntimeError('could not launch zookeeper for {} seconds'.format(timeout))


def parse_argv(argv):
    parser = argparse.ArgumentParser()
    parser.add_argument('-z', '--zookeeper-port', type=int, help='Port for Zookeeper')

    parser.add_argument(
        '--tick-time', dest='tick_time', action='store',
        type=int, default=20000,
        help=""
    )

    parser.add_argument(
        '--max-session-timeout', dest='max_session_timeout', action='store',
        type=int, default=400000,
        help=""
    )

    return parser.parse_args(argv)


def start(argv):
    extract(yatest.common.binary_path(
        'library/recipes/zookeeper/package/{}.tar.gz'.format(ZOOKEEPER_DIRECTORY)))

    args = parse_argv(argv)
    host = '127.0.0.1'
    if args.zookeeper_port:
        port = args.zookeeper_port
    else:
        pm = yatest.common.network.PortManager()
        port = pm.get_tcp_port()
    conn_string = '{}:{}'.format(host, port)

    log('Will use port {}'.format(port))
    write_config(port, args.tick_time, args.max_session_timeout)
    log('Starting zookeeper...')
    run_command('start')
    wait_for_start(port)
    set_env('RECIPE_ZOOKEEPER_HOST', host)
    set_env('RECIPE_ZOOKEEPER_PORT', str(port))
    set_env('RECIPE_ZOOKEEPER_CONNSTRING', conn_string)


def stop(argv):
    log('Stopping zookeeper...')
    try:
        run_command('stop')
    except OSError:
        pass
