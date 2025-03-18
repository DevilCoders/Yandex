#!/usr/bin/python -u
import os
import urllib2
import json
import subprocess
import socket
import logging
import re
import time
import random

from antiadblock.libs.deploy_unit_resolver.lib import get_fqdns

# Skynet constants
HOST_SKYNET_PATH = '/Berkanavt/supervisor/base/active'
SKY_BIN_PATH = '/skynet/tools/sky'
CONTAINER_SKYNET_PATH = '/skynet'
CONTAINER_SKY_PATH = '/usr/local/bin/sky'


RESOURSE_TYPE = 'ANTIADBLOCK_LAUNCH_FILEBEAT'

# File paths
WORK_PATH = '/log_shippers/filebeat'
RESOURSE_IDS_FILE = os.path.join(WORK_PATH, 'resourse_id.txt')
START_FILEBEAT_FILE = os.path.join(WORK_PATH, 'if_launch.json')
ELASTIC_HOSTS_FILE = os.path.join(WORK_PATH, 'elastic_hosts.txt')

logging.basicConfig(format='%(asctime)s %(levelname)-8s %(message)s',
                    filename=os.path.join(WORK_PATH, "update_filebeat.log"), level=logging.INFO, datefmt='%Y-%m-%d %H:%M:%S')

API_URL_TEMPLATE = 'https://sandbox.yandex-team.ru/api/v1.0/resource?limit=1&type={type}&state=READY'

PORT = '8890'

# https://wiki.yandex-team.ru/skynet/drafts/edinyjj-xostovyjj-skynet/
def configure_host_sky():
    if not os.path.exists(CONTAINER_SKYNET_PATH):
        os.symlink(HOST_SKYNET_PATH, CONTAINER_SKYNET_PATH)
    if not os.path.exists(CONTAINER_SKY_PATH):
        os.symlink(SKY_BIN_PATH, CONTAINER_SKY_PATH)

def read_from_file(filename):
    if os.path.exists(filename):
        with open(filename, 'r') as f:
            return f.read().splitlines()
    return None

def write_to_file(filename, array):
    with open(filename, 'w') as f:
        f.write('\n'.join(list(map(str, array))))

def read_json(filename):
    if os.path.exists(filename):
        with open(filename, 'r') as f:
            return json.load(f)
    return None

def get_sandbox_resourse_id(resourse_type):
    response = urllib2.urlopen(API_URL_TEMPLATE.format(type=resourse_type))
    resource_description = json.loads(response.read())
    items = resource_description['items']
    if len(items):
        return items[0]['id']
    return None


def download_sandox_resourse(resourse_file_path, id):
    logging.info('Loading sandbox_resourse ' + str(id))
    subprocess.call([CONTAINER_SKY_PATH, 'get', '-d', resourse_file_path, 'sbr:{}'.format(id)])


def pkill_filebeat(cryprox_filebeat, nginx_filebeat):
    try:
        for filebeat in [cryprox_filebeat, nginx_filebeat]:
            if filebeat is not None:
                filebeat.kill()
        logging.info('Filebeats are killed')
    except:
        logging.exception('Filebeat is already killed or have never been started')

    return None, None

def get_host_cluster():
    HOSTNAME_TEMPL = r'(.*?\.)?(?P<smth>(?P<dc>\w+)\.yp-c\.yandex\.net)'
    HOSTNAME_RE = re.compile(HOSTNAME_TEMPL)
    m = HOSTNAME_RE.match(socket.gethostname())
    return m.group('dc')

def deploy_unit_resolver(port, fqdns):
    fqdns = map(lambda x: x + ':' + port, fqdns)
    return ",".join(fqdns)

def launch_filebeat(filebeat_type, fqdns):
    """ Filebeat_type = cryprox or nginx"""
    logging.info('Launching filebeat ' + filebeat_type)
    child_env = os.environ.copy()
    fqdns = list(fqdns)
    random.shuffle(fqdns)
    child_env['ELASTIC_HOSTS'] = deploy_unit_resolver(PORT, fqdns)
    child_env['FILEBEAT_TAGS'] = '[' + filebeat_type + ", 'yp']"
    cmd = ['filebeat', '-path.config', os.path.join(WORK_PATH, filebeat_type), '-path.data', '/logs/filebeat-' + filebeat_type,  '-strict.perms=false',  '-v']
    return subprocess.Popen(cmd, env=child_env)

def start_filebeat(elastic_hosts_file, fqdns):
    cryprox_filebeat = launch_filebeat('cryprox', fqdns)
    nginx_filebeat = launch_filebeat('nginx', fqdns)
    write_to_file(elastic_hosts_file,  fqdns)
    return cryprox_filebeat, nginx_filebeat

def fqdns(elastic_hosts_file):
    old_fqdns = read_from_file(elastic_hosts_file)
    try:
        new_fqdns = get_fqdns('antiadb-elasticsearch', ['DataNodes'], ['vla', 'sas', 'man'])
    except:
        new_fqdns = old_fqdns
        logging.error("Deploy unit resolver doesn't respond, can't get fqdns, using old ones")

    return old_fqdns, new_fqdns

def checkout_elastic_hosts(elastic_hosts_file,  cryprox_filebeat, nginx_filebeat):
    old_fqdns, new_fqdns = fqdns(elastic_hosts_file)
    if old_fqdns != new_fqdns:
        logging.info("Hosts changed")
        cryprox_filebeat, nginx_filebeat = pkill_filebeat(cryprox_filebeat, nginx_filebeat)
        cryprox_filebeat, nginx_filebeat = start_filebeat(elastic_hosts_file, new_fqdns)
    else:
        cryprox_filebeat, nginx_filebeat = checkout_filebeat_process(new_fqdns, cryprox_filebeat, nginx_filebeat)

    return cryprox_filebeat, nginx_filebeat

def checkout_filebeat_process(fqdns, cryprox_filebeat, nginx_filebeat):
    if cryprox_filebeat is None or cryprox_filebeat.poll() is not None:
        cryprox_filebeat = launch_filebeat('cryprox', fqdns)
    if nginx_filebeat is None or nginx_filebeat.poll() is not None:
        nginx_filebeat = launch_filebeat('nginx', fqdns)
    return cryprox_filebeat, nginx_filebeat


def checkout_filebeat(cryprox_filebeat, nginx_filebeat):
    new_sandbox_id = get_sandbox_resourse_id(RESOURSE_TYPE)
    old_sandbox_id = int(read_from_file(RESOURSE_IDS_FILE)[0]) if read_from_file(RESOURSE_IDS_FILE) else None
    if old_sandbox_id != new_sandbox_id:
        logging.info("Configuration changed")
        if new_sandbox_id:
            write_to_file(RESOURSE_IDS_FILE, [new_sandbox_id])
            download_sandox_resourse(WORK_PATH, new_sandbox_id)
        cryprox_filebeat, nginx_filebeat = pkill_filebeat(cryprox_filebeat, nginx_filebeat)
        if int(read_json(START_FILEBEAT_FILE)[get_host_cluster()]):
            _, new_fqdns = fqdns(ELASTIC_HOSTS_FILE)
            cryprox_filebeat, nginx_filebeat = start_filebeat(ELASTIC_HOSTS_FILE, new_fqdns)
    else:
        logging.info("Same configuration")
        if int(read_json(START_FILEBEAT_FILE)[get_host_cluster()]):
            cryprox_filebeat, nginx_filebeat = checkout_elastic_hosts(ELASTIC_HOSTS_FILE, cryprox_filebeat, nginx_filebeat)

    return cryprox_filebeat, nginx_filebeat


def __main__():
    configure_host_sky()
    cryprox_filebeat = None
    nginx_filebeat = None
    while True:
        cryprox_filebeat, nginx_filebeat = checkout_filebeat(cryprox_filebeat, nginx_filebeat)
        time.sleep(60)


if __name__ == "__main__":
    __main__()

# starts by crontab
# if resourse_changed
    # try_pkill
    # if launch==1
         # restart
# not_changed
    # if launch==1
        # checkout_elastic_host

