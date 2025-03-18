import resource

CONFIG_BY_KEY = {
    0: {
        'is_prod': True,
        'yav_secret_id': 'sec-01d54hswpczgqhn0kjb9161hm2',
        'yav_api_tvm_id_name': 'configs_api_tvm_id',
        'groups': {
            'gencfg@SAS_AAB_MONRELAY': {'tag': 'trunk'},
            'gencfg@VLA_AAB_MONRELAY': {'tag': 'trunk'},
            'gencfg@MAN_AAB_MONRELAY': {'tag': 'trunk'},
        },
        'yp_resources': {
            'SAS': {
                'cpu_guarantee': 4000,  # mcores
                'cpu_limit': 4000,  # mcores
                'memory_guarantee': 4096,  # MB (at least 1536 for samogon infra)
                'memory_limit': 8096,  # MB
                'replicas': 1,
                'rootfs': 20 * 1024,  # MB
                'storage': 50 * 1024,  # MB (size of /storage volume)
                # 'ssd': 20480,  # optional # MB (size of /ssd volume) [by default ssd volume will not be allocated] Since v70
                'network_macro': '_ANTIADBNETS_',
                # 'network_macro': '_SEARCHPRODNETS_',
            },
        }
    },
    1: {
        'is_prod': False,
        'yav_secret_id': 'sec-01d54hswpczgqhn0kjb9161hm2',
        'yav_api_tvm_id_name': 'configs_api_tvm_id',
        'groups': {
            'gencfg@SAS_AAB_MONRELAY': {'tag': 'trunk'},
            'gencfg@VLA_AAB_MONRELAY': {'tag': 'trunk'},
            'gencfg@MAN_AAB_MONRELAY': {'tag': 'trunk'},
        },
        'yp_resources': {
            'SAS': {
                'cpu_guarantee': 4000,  # mcores
                'cpu_limit': 4000,  # mcores
                'memory_guarantee': 4096,  # MB (at least 1536 for samogon infra)
                'memory_limit': 8096,  # MB
                'replicas': 1,
                'rootfs': 20 * 1024,  # MB
                'storage': 50 * 1024,  # MB (size of /storage volume)
                # 'ssd': 20480,  # optional # MB (size of /ssd volume) [by default ssd volume will not be allocated] Since v70
                'network_macro': '_ANTIADBNETS_',
                # 'network_macro': '_SEARCHPRODNETS_',
            },
        }
    }
}


class MonRelay(object):
    __name__ = 'MonRelay'
    __cfg_fmt__ = 'json'
    LOG_PATH = '{srv}/monreal.log'

    def __init__(self, yt_token, monrelay_tvm_id, configs_api_tvm_id, configs_api_tvm_secret, key, configs_api_host, yql_token):
        self.__cfg__ = {
            'yt_token': yt_token,
            'yql_token': yql_token,
            'monrelay_tvm_id': monrelay_tvm_id,
            'configs_api_tvm_id': configs_api_tvm_id,
            'monrelay_tvm_secret': configs_api_tvm_secret,
            'key': key,
            'configs_api_host': configs_api_host
        }

    def start(self):
        self.__cfg__['log_path'] = self.substval(self.LOG_PATH)
        resources = {resource.RLIMIT_NOFILE:
                         (102400, 204800), resource.RLIMIT_CORE: (-1, -1)}
        self.create([self.find('monrelay'), '-c', self.config_as_file()], resources=resources)

    def packages(self):
        return [
            'monrelay.tar.gz',
        ]

    def logfiles(self):
        return [
            {
                'path': self.LOG_PATH,
                'maxBytes': 100 * 1024 * 1024,
                'backupCount': 5,
            }
        ]


class Plugin(object):
    __name__ = 'aab_monrelay'

    def __init__(self, iface):
        self._iface = iface
        self._key = self._iface.key()
        self._config = CONFIG_BY_KEY[self._key]

    def conf(self):
        return {
            'key': self._key,
            'groups': self._config['groups'],
            # 'abc_service_id': 1526,  # REQUIRED for yp-lite quota
            # 'yp_lite_allocation': self._config['yp_resources']
        }

    def gencluster(self, iface):
        secrets = iface.secrets[self._config['yav_secret_id']]
        yt_token = iface.samogon_token
        yql_token = secrets['yql_token']
        monrelay_tvm_id = secrets['monrelay_tvm_id']
        configs_api_tvm_secret = secrets['configs_api_tvm_secret']
        configs_api_tvm_id = secrets[self._config['yav_api_tvm_id_name']]
        configs_api_host = self._config.get('configs_api_host', 'api.aabadmin.yandex.ru')
        for host, info in iface.host_list():
            yield host, MonRelay(yt_token=yt_token,
                                 monrelay_tvm_id=monrelay_tvm_id,
                                 configs_api_tvm_id=configs_api_tvm_id,
                                 configs_api_tvm_secret=configs_api_tvm_secret,
                                 key=self._key,
                                 configs_api_host=configs_api_host,
                                 yql_token=yql_token)

    def moderators(self):
        return ['evor', 'abc:antiadblock']

    def is_test(self):
        return not self._config['is_prod']
