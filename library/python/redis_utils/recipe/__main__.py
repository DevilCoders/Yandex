import os
import signal
import time
import tarfile
import subprocess
from pathlib import Path

from library.python.testing.recipe import declare_recipe, set_env
from yatest.common.network import PortManager


REDIS_BIN = Path('redis-server')
PID_FILE = Path('recipe.pid')


def start(argv: list):
    untar_redis()
    pm = PortManager()
    port = pm.get_port()
    start_redis(port)


def start_redis(port: int):
    if PID_FILE.exists():
        return
    process = subprocess.Popen([f'./{REDIS_BIN}', '--port', str(port)])
    open(f'{PID_FILE}{port}', 'w').write(str(process.pid))
    set_envs(port)
    time.sleep(0.1)


def stop(argv):
    pid = int(open(f'{PID_FILE}{os.getenv("REDIS_PORT")}').read())
    os.kill(pid, signal.SIGTERM)


def set_envs(port: int):
    set_env('REDIS_PORT', str(port))
    set_env('REDIS_HOSTS', '["localhost"]')
    set_env('REDIS_CLUSTER_NAME', 'autotest')
    set_env('REDIS_PASSWORD', '-')


def untar_redis():
    if REDIS_BIN.exists():
        return
    tarfile.open('resource.tar.gz').extractall()


if __name__ == "__main__":
    declare_recipe(start, stop)
