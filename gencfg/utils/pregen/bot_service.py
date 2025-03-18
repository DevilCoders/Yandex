#!/skynet/python/bin/python

"""Script to download hosts hardware info from bot.yandex-team.ru. See usage for more details."""

import os
import re
import sys
import threading
import urllib2

sys.path.append(os.path.abspath(os.path.join(os.path.dirname(__file__), '..', '..')))

import gencfg
from core.db import CURDB

DIR = os.path.join(CURDB.PATH, '.bot')
FILE = os.path.join(CURDB.PATH, 'bot_service_data')


def gather_single(host):
    try:
        page = urllib2.urlopen('http://bot.yandex-team.ru/api/consistof.php?name=%s.yandex.ru' % host).read()
    except:
        page = None
    return page


def gather():
    class WorkingThread(threading.Thread):
        def __init__(self, hosts, dir):
            threading.Thread.__init__(self)
            self.hosts = hosts
            self.dir = dir

        def run(self):
            for host in self.hosts:
                page = gather_single(host)
                if page is None:
                    continue
                path = os.path.join(self.dir, host)
                with open(path, 'w') as f:
                    f.write(page)
                    f.close()

    if not os.path.exists(DIR):
        os.mkdir(DIR)

    hosts = open(os.path.join('..', CURDB.HDATA_DIR, 'hosts_data')).readlines()
    hosts = [host.rstrip().split('\t')[0] for host in hosts]
    hosts = [host for host in hosts if not os.access(os.path.join(DIR, host), os.R_OK)]

    workers = []
    wcount = 10
    for i in range(wcount):
        fst = (i * len(hosts)) / wcount
        lst = ((i + 1) * len(hosts)) / wcount
        workers.append(WorkingThread(hosts[fst:lst], DIR))
    for worker in workers:
        worker.start()
    for worker in workers:
        worker.join()


def parse(host, page, models):
    if page.startswith('err: not found'):
        return None
    page = page.replace('\n', '').replace('\r', '').replace('<br>', '\n').replace('<br/>', '\n').replace('<br />', '\n')
    lines = page.split('\n')
    lines = [line.strip() for line in lines]
    lines = [line for line in lines if line]
    drivereg = re.compile('\d+ HDD(.+)\((.+)\) S/N.*')
    drives = []
    for line in lines:
        searchres = drivereg.search(line)
        if not searchres:
            continue
        # if line.find('SSD') != -1:
        #    host = os.path.basename(path)
        model = searchres.group(1).strip()
        line = searchres.group(2)
        items = line.split('/')
        size = items[0]
        if size.endswith('Tb') or size.endswith('TB'):
            size = str(int(size[:-2]) * 1024) + 'GB'
        if not (size.endswith('GB') or size.endswith('Gb')):
            print 'Parse error at %s' % host
            print size
            print page
            assert False
        size = int(size[:-2])
        type = 'SSD' if items[2] == 'SSD' else 'HDD'
        drives.append((type, size))
        models.add(model)

    ssds = [size for (type, size) in drives if type == 'SSD']
    ssd_size = sum(ssds, 0)
    hdds = [size for (type, size) in drives if type == 'HDD']
    hdd_count = len(hdds)
    info = {'host': str(host), 'ssd_size': str(ssd_size), 'hdd_count': str(hdd_count)}

    return info


def compose():
    hosts = os.listdir(DIR)
    infos = []
    headers = ['host', 'ssd_size', 'hdd_count']
    models = set()
    for host in hosts:
        path = os.path.join(DIR, host)
        page = open(path).read()
        info = parse(host, page, models)
        if info is None:
            continue
        info = [info[key] for key in headers]
        infos.append(info)
    for model in models:
        print model
    infos.sort()
    lines = ['\t'.join(info) for info in infos]
    with open(FILE, 'w') as f:
        print >> f, '\n'.join(['\t'.join(headers)] + lines)
        f.close()


if __name__ == '__main__':
    if len(sys.argv) == 2 and sys.argv[1] == 'gather':
        gather()
        exit(0)
    if len(sys.argv) == 2 and sys.argv[1] == 'compose':
        compose()
        exit(0)
    print ('Usage:' +    \
           '\tbot_service gather\t- to gather data files to %s/ (in about 10-15 minutes)\n' + \
           '\tbot_service compose\t- to compose files from %s/ to %s file\n' + \
           '\tall data is in %s') % \
          (os.path.basename(DIR), os.path.basename(DIR), os.path.basename(FILE), CURDB.HDATA_DIR)
    exit(-1)
