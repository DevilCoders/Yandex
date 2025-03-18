import getpass
import logging
import os
import json
import sys
import signal
import time
import subprocess


PROXY_CONDUCTOR_TAG = 'arcadia_api_proxy'
CONVERTER_CONDUCTOR_TAG = 'arcadia_api_robot'

HG_SERVER_PACKAGE = {
    'type': 'naked',
    'source': 'sbr:1025412056',
    'alias': 'hg-server',
}


def unpack_naked(servant, package_name):
    path = servant.get_package(package_name)
    for f in os.listdir(path):
        if f.endswith('.tar.gz'):
            f = os.path.join(path, f)
            if subprocess.call(['tar', '-C', path, '-xzf', f]) != 0:
                raise Exception('failed to unpack {}'.format(f))
    return path


def terminate(p, timeout):
    p.terminate()
    t = time.time()

    while p.executing() and time.time() - t < timeout:
        p.terminate()
        time.sleep(1)

    if p.executing():
        p.kill()


class Proxy(object):
    __name__ = 'vcs_api_proxy'
    __cfg_fmt__ = 'json'

    def __init__(self, host, cfg):
        self.__cfg__ = {
            'disc_store_path': self.store_path(),
            'ram_store_size': 1000000,
            'host': host,
            'port': self.port(),
            'async_port': self.async_port(),
            'sensors_port': self.sensors_port(),
            'debug_output': 1,
            'eventlog_path': '{srv}/event.log',  # TODO rotate
            'inner_pool_size': 48,
            'async_server_threads': 8,
            'warmup': {
                'paths': [
                    '/trunk/arcadia/util',
                    '/trunk/arcadia/library',
                    '/trunk/arcadia/kernel',
                    '/trunk/arcadia/contrib',
                    '/trunk/arcadia',
                ],
            },
            'hg_path': self.hg_path(),
            'hg_server': 'ssh://arcadia-hg.yandex-team.ru/arcadia.hg',
            'hg_user': 'arcadia-devtools',
            'hg_key': self.key(),
        }
        self.__cfg__.update(cfg)


    def postinstall(self):
        hg_server_path = os.path.join(unpack_naked(self, 'hg-server'), 'hg')
        self.writetofile(
            '#!/bin/bash\n{} "$@"'.format(os.path.join(hg_server_path, 'hg')),
            self.hg_path(),
            chmod=0755
        )
        self.copyfile(self.find('vcs'), '/usr/bin/aapi_client', chmod=0755, chown='root')
        self.writetofile(self.__cfg__['key'], self.key(), chmod=0600)

    def packages(self):
        return ['proxy.tgz', 'client.tgz', HG_SERVER_PACKAGE]

    def hg_path(self):
        return '/storage/arcadia_api_proxy/hg'

    def key(self):
        return '{srv}/.key'

    def start(self):
        self.create([self.find('proxy'), '--config-path', self.config_as_file()])

    @staticmethod
    def port():
        return 6666

    @staticmethod
    def sensors_port():
        return 6677

    @staticmethod
    def async_port():
        return 7777

    @staticmethod
    def store_path():
        return '/storage/arcadia_api_proxy/store'

    def sensors_url(self):
        return '{port}/json'.format(port=self.__cfg__.get('monitor_port', 6699))

    def user(self):
        return self.uniq_persist_user(self.__name__)


class HgConverter(object):
    __name__ = 'vcs_api_hg_converter'
    __cfg_fmt__ = 'json'

    def __init__(self, host, proxies, cfg):
        self.__cfg__ = {
            'hg_repo_path': '/storage/arcadia_api_converter/hg/arcadia',
            'hg_binary_path': self.hg_path(),
            'hg_user': 'arcadia-devtools',
            'hg_key': self.key(),
            'vcs_api_services': proxies,
            'vcs_local_store_path': self.store_path(),
            'hg_uploaded_changesets_path': '/storage/arcadia_api_converter/hg/changesets_cache',
            'garbage_dir': self.garbage_path(),
            'host': host,
        }
        self.__cfg__.update(cfg)

    def create_file(self, path, contents, mode=0644):
        uid = self.user()['uid']
        gid = self.user()['gid']

        with open(path, 'w') as f:
            if type(contents) is list:
                contents = '\n'.join(contents)
            f.write(contents)
            f.write('\n')
            os.fchmod(f.fileno(), mode)
            os.fchown(f.fileno(), uid, gid)

    def postinstall(self):
        hg_server_path = os.path.join(unpack_naked(self, 'hg-server'), 'hg')
        self.create_file(self.hg_path(), [
            "#!/bin/bash",
            "{} \"$@\"".format(os.path.join(hg_server_path, 'hg')),
        ], mode=0755)
        self.writetofile(self.__cfg__['key'], self.key(), chmod=0600)

    def key(self):
        return '{srv}/.key'

    def packages(self):
        return ['hg_robot.tgz', HG_SERVER_PACKAGE]

    def start(self):
        env = {
            'LANG': 'en_US.UTF-8',
            'LANGUAGE': '',
            'LC_CTYPE': 'en_US.UTF-8',
            'LC_NUMERIC': 'en_US.UTF-8',
            'LC_TIME': 'en_US.UTF-8',
            'LC_COLLATE': 'en_US.UTF-8',
            'LC_MONETARY': 'en_US.UTF-8',
            'LC_MESSAGES': 'en_US.UTF-8',
            'LC_PAPER': 'en_US.UTF-8',
            'LC_NAME': 'en_US.UTF-8',
            'LC_ADDRESS': 'en_US.UTF-8',
            'LC_TELEPHONE': 'en_US.UTF-8',
            'LC_MEASUREMENT': 'en_US.UTF-8',
            'LC_IDENTIFICATION': 'en_US.UTF-8',
            'LC_ALL': 'en_US.UTF-8',
        }
        self.create([self.find('robot'), '-c', self.config_as_file()], env=env)

    def stop(self):
        for p in self.procs():
            try:
                process.stopRetries()
            except Exception as e:
                logging.error('error in stopRetries: %s', e)

        for p in self.procs():
            try:
                terminate(p, 180)
            except Exception as e:
                logging.error('error in terminate process: %s', e)

    def store_path(self):
        return '/storage/arcadia_api_converter/store'

    def garbage_path(self):
        return '/storage/arcadia_api_converter/garbage'

    def hg_path(self):
        return '/storage/arcadia_api_converter/hg/hg'

    def user(self):
        return {
            'name': 'aapi_converter',
            'uid': 1100,
            'raise_uid_conflict': False,
            'group': 'cauth-users',
            'gid': 15513,
            'raise_gid_conflict': False,
            'create_home': True,
        }


class SvnConverter(object):
    __name__ = 'vcs_api_svn_converter'
    __cfg_fmt__ = 'json'

    def __init__(self, host, proxies, cfg):
        self.__cfg__ = {
            'svn_local_repo': '/storage/arcadia/arc',
            'vcs_api_services': proxies,
            'vcs_local_store_path': self.store_path(),
            'garbage_dir': self.garbage_path(),
            'host': host,
            'sensors_port': self.sensors_port(),
            'blacklist': [
                '/robots',
                '/robots.DONT-USE-IT--CONTENT-WAS-MOVED-TO-ARCADIA.YANDEX.RU-ROBOTS',
                '/yabs/trunk/umbrella/bscoll/common/Yabs/AntiFraud/Filters',
                '/yabs/trunk/umbrella/bscoll/common/filters',
                '/yabs/trunk/umbrella/bscoll/common/ft/filters',
                '/yabs/branches/bscoll',
                '/yabs/task/bscoll',
                '/yabs/trunk/umbrella/yabs-bscoll-cluster-control/ssh',
                '/yabs/branches/yabs-bscoll-cluster-control',
                '/yabs/trunk/umbrella/bsistat_antifraud/Yabs/AntiFraud/DropStatFilters',
                '/yabs/trunk/umbrella/bsistat_antifraud/Yabs/AntiFraud/NetFilters',
                '/yabs/branches/bsistat_antifraud',
                '/yabs/trunk/umbrella/yabs-auto-supbs/Yabs/AutoSupBS/FraudCriteria',
                '/yabs/branches/yabs-auto-supbs',
                '/yabs/trunk/umbrella/mapreduce_antifraud',
                '/yabs/branches/mapreduce_antifraud',
                '/yabs/trunk/umbrella/yabs-bscoll-openstack/ssh',
                '/yabs/branches/yabs-bscoll-openstack',
                '/yabs/trunk/umbrella/yabs-bscoll-filters',
                '/yabs/branches/yabs-bscoll-filters',
                '/yabs/users/ijon/barleycorn',
                '/yabs/trunk/barleycorn',
                '/yabs/branches/barleycorn',
                '/secure',
                '/secure-test',
                '/trunk/arcadia/yabs/server-dev-depends',
            ]
        }
        self.__cfg__.update(cfg)

    def packages(self):
        return ['converter.tgz']

    def start(self):
        env = {
            'LANG': 'en_US.UTF-8',
            'LANGUAGE': '',
            'LC_CTYPE': 'en_US.UTF-8',
            'LC_NUMERIC': 'en_US.UTF-8',
            'LC_TIME': 'en_US.UTF-8',
            'LC_COLLATE': 'en_US.UTF-8',
            'LC_MONETARY': 'en_US.UTF-8',
            'LC_MESSAGES': 'en_US.UTF-8',
            'LC_PAPER': 'en_US.UTF-8',
            'LC_NAME': 'en_US.UTF-8',
            'LC_ADDRESS': 'en_US.UTF-8',
            'LC_TELEPHONE': 'en_US.UTF-8',
            'LC_MEASUREMENT': 'en_US.UTF-8',
            'LC_IDENTIFICATION': 'en_US.UTF-8',
            'LC_ALL': 'en_US.UTF-8',
        }
        self.create([self.find('convert'), '-c', self.config_as_file()], env=env)

    def store_path(self):
        return '/storage/arcadia_api_converter/store'

    def garbage_path(self):
        return '/storage/arcadia_api_converter/garbage'

    def user(self):
        return {
            'name': 'aapi_converter',
            'uid': 1100,
            'raise_uid_conflict': False,
            'group': 'cauth-users',
            'gid': 15513,
            'raise_gid_conflict': False,
            'create_home': True,
        }

    @staticmethod
    def sensors_port():
        return 6678

    def sensors_url(self):
        return '{port}/sensors'.format(port=self.sensors_port())


class Plugin(object):
    __name__ = 'vcs_api'

    def __init__(self, iface):
        self._user = iface.conf().get('user') or getpass.getuser()
        assert iface.key() in (4, 7)
        self._is_production = iface.key() == 7  # TODO production conventional key is 0

    def moderators(self):
        return ['abc:arc', 'akastornov', 'artanis', 'stanly', 'nslus', 'snowball', 'deshevoy', 'kikht', 'spreis']

    def conf(self):
        if self._is_production:
            return {
                'key': 6,
                'groups': [
                    'CONDUCTORTAG@{}'.format(PROXY_CONDUCTOR_TAG),
                    'CONDUCTORTAG@{}'.format(CONVERTER_CONDUCTOR_TAG)
                ],
                'root_partition': '/place/' + self.__name__
            }
        else:
            return {
                'key': 4,
                'hosts': [
                    'arcadia-hg02.search.yandex.net',
                ]
            }

    def gencluster(self, iface):
        secrets = iface.secrets.get('sec-01ddbfeqnqa7d6mac4bmttr9np')

        secrets_cfg = {
            'key': secrets['key'],
            'yt_token': secrets['yt_token'],
            'ydb_token': secrets['ydb_token'],
        }

        if self._is_production:
            cfg = {
                'yt_proxy': 'markov',
                'yt_table': '//home/devtools/vcs/data',
                'yt_svn_path': '//home/devtools/vcs/svn_head',
                'svn_head_yt_path': '//home/devtools/vcs/svn_head',
                'ydb_endpoint': 'ydb-ru.yandex.net:2135',
                'ydb_table': '/ru/devtools/prod/aapi',
            }
            cfg.update(secrets_cfg)

            proxy_hosts = []
            svn_converter_hosts = []
            hg_converter_hosts = []

            for host, info in iface.host_list():
                if PROXY_CONDUCTOR_TAG in info['conductorTags']:
                    proxy_hosts.append(host)

                if CONVERTER_CONDUCTOR_TAG in info['conductorTags']:
                    svn_converter_hosts.append(host)
                    hg_converter_hosts.append(host)

            for host in proxy_hosts:
                yield host, Proxy(host, cfg)

            proxies = [p + ':' + str(Proxy.async_port()) for p in proxy_hosts]

            for host in svn_converter_hosts:
                yield host, SvnConverter(host, proxies, cfg)

            for host in hg_converter_hosts:
                yield host, HgConverter(host, proxies, cfg)

        else:
            cfg = {
                'yt_proxy': 'hume',
                'yt_table': '//home/devtools/vcs/data',
                'yt_svn_path': '//home/devtools/vcs/svn_head',
                'svn_head_yt_path': '//home/devtools/vcs/svn_head',
            }
            cfg.update(secrets_cfg)

            for host, info in iface.host_list():
                yield host, Proxy(host, cfg)
                yield host, SvnConverter(host, [host + ':' + str(Proxy.async_port())], cfg)
