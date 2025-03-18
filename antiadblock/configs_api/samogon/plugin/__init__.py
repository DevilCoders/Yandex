import subprocess
from subprocess import Popen, PIPE, STDOUT
import os
import urllib2
import json
import logging

AAB_CRYPT_KEY_SECRET_ID = 'sec-01daxgp7fks6j2trfxyjt9cxgf'
AAB_ADMIN_TVM_SECRET_ID = 'sec-01daxgyeftvjqn405dgnyqc5sx'
AAB_ADMIN_TOOLS_TOKEN_ID = 'sec-01e32cs8h9ystjc7v2wjf713m7'
DB_YAV_SECRET_ID = 'sec-01d1gdj91zqghj9fbyvdrmey4g'
CHARTS_YAV_SECRET_ID = 'sec-01dn2bbysse2gc59rn6kdbhmxz'
SANDBOX_YAV_SECRET_ID = 'sec-01dpnswx6gjzgmbejetzhgs136'
INFRA_YAV_SECRET_ID = 'sec-01f1yfezy610vnfxat7bfhr25p'
CONFIG_BY_KEY = {
    0: {
        'is_prod': True,
        'db_yav_secret_key': 'aab-admin-db-password',
        'charts_yav_secret_key': 'antiadb_charts_token',
        'sandbox_yav_secret_key': 'antiadb_sandbox_token',
        'infra_yav_secret_key': 'prod_token',
        'aab_admin_tvm_secret_key': 'prod_key',
        'aab_admin_tools_token_key': 'prod_key',
        'aab_crypt_key_secret_key': 'prod_key',
        'yp_resources': {
            'cpu_guarantee': 4000,  # mcores
            'cpu_limit': 4000,  # mcores
            'memory_guarantee': 8096,  # MB (at least 1536 for samogon infra)
            'memory_limit': 8096,  # MB
            'replicas': 1,
            'rootfs': 20 * 1024,  # MB
            'root_bandwidth_guarantee_megabytes_per_sec': 5,
            'root_bandwidth_limit_megabytes_per_sec': 10,
            'volumes': [
                {
                    'disk_quota_megabytes': 50 * 1024,
                    'mount_point': '/storage',
                    'storage_class': 'hdd',
                    'bandwidth_guarantee_megabytes_per_sec': 10,
                    'bandwidth_limit_megabytes_per_sec': 20,
                },
            ],
            'network_macro': '_ANTIADBNETS_',
        },
        'dcs': ['SAS', 'IVA', 'MYT', 'VLA'],

        'pg_db_host': 'vla-qbwmldgp59lcwwr6.db.yandex.net,sas-w9jnn4x6osfkn7nh.db.yandex.net',
        'pg_db_port': '6432',
        'pg_db_name': 'aab_admin_yandex_team_ru_db',

        'ENVIRONMENT_TYPE': 'PRODUCTION',
        'HOST_DOMAIN': 'antiblock.yandex.ru',
        'WEBMASTER_API_URL': 'https://webmaster3-internal.prod.in.yandex.net',
        'INFRA_API_URL': 'https://infra-api.yandex-team.ru/v1/',
    },
    11: {
        'is_prod': False,
        'db_yav_secret_key': 'aab-admin-db-password-preprod',
        'charts_yav_secret_key': 'antiadb_charts_token',
        'sandbox_yav_secret_key': 'antiadb_sandbox_token',
        'infra_yav_secret_key': 'test_token',
        'aab_admin_tvm_secret_key': 'test_key',
        'aab_admin_tools_token_key': 'test_key',
        'aab_crypt_key_secret_key': 'test_key',
        'yp_resources': {
            'cpu_guarantee': 2000,  # mcores
            'cpu_limit': 2000,  # mcores
            'memory_guarantee': 4096,  # MB (at least 1536 for samogon infra)
            'memory_limit': 4096,  # MB
            'replicas': 1,
            'rootfs': 10 * 1024,  # MB
            'root_bandwidth_guarantee_megabytes_per_sec': 1,
            'root_bandwidth_limit_megabytes_per_sec': 2,
            'volumes': [
                {
                    'disk_quota_megabytes': 20 * 1024,
                    'mount_point': '/storage',
                    'storage_class': 'hdd',
                    'bandwidth_guarantee_megabytes_per_sec': 1,
                    'bandwidth_limit_megabytes_per_sec': 2,
                },
            ],
            'network_macro': '_ANTIADBNETS_',
        },
        'dcs': ['SAS'],

        'pg_db_host': 'vla-63ocvgqmyv2umpn7.db.yandex.net',
        'pg_db_port': '6432',
        'pg_db_name': 'aab_admin_yandex_team_ru_db_preprod',

        'ENVIRONMENT_TYPE': 'TESTING',
        'HOST_DOMAIN': 'preprod.antiblock.yandex.ru',
        'WEBMASTER_API_URL': 'https://webmaster3-internal.test.in.yandex.net',
        'INFRA_API_URL': 'https://infra-api-test.yandex-team.ru/v1/',
    },
    33: {
        'is_prod': False,
        'db_yav_secret_key': 'aab-admin-dev_db-password',
        'charts_yav_secret_key': 'antiadb_charts_token',
        'sandbox_yav_secret_key': 'antiadb_sandbox_token',
        'infra_yav_secret_key': 'test_token',
        'aab_admin_tvm_secret_key': 'test_key',
        'aab_admin_tools_token_key': 'test_key',
        'aab_crypt_key_secret_key': 'test_key',
        'yp_resources': {
            'cpu_guarantee': 1000,  # mcores
            'cpu_limit': 2000,  # mcores
            'memory_guarantee': 4096,  # MB (at least 1536 for samogon infra)
            'memory_limit': 4096,  # MB
            'replicas': 1,
            'rootfs': 10 * 1024,  # MB
            'root_bandwidth_guarantee_megabytes_per_sec': 1,
            'root_bandwidth_limit_megabytes_per_sec': 2,
            'volumes': [
                {
                    'disk_quota_megabytes': 20 * 1024,
                    'mount_point': '/storage',
                    'storage_class': 'hdd',
                    'bandwidth_guarantee_megabytes_per_sec': 1,
                    'bandwidth_limit_megabytes_per_sec': 2,
                },
            ],
            'network_macro': '_ANTIADBNETS_',
        },
        'dcs': ['VLA'],

        'pg_db_host': 'vla-0tltgs8bscz6pzen.db.yandex.net',
        'pg_db_port': '6432',
        'pg_db_name': 'aab_admin_yandex_team_ru_db_dev',

        'ENVIRONMENT_TYPE': 'TESTING',
        'HOST_DOMAIN': 'develop.antiblock.yandex.ru',
        'WEBMASTER_API_URL': 'https://webmaster3-internal.test.in.yandex.net',
        'INFRA_API_URL': 'https://infra-api-test.yandex-team.ru/v1/',
    },
    44: {
        'is_prod': False,
        'db_yav_secret_key': 'aab-admin-dev_db-password',
        'charts_yav_secret_key': 'antiadb_charts_token',
        'sandbox_yav_secret_key': 'antiadb_sandbox_token',
        'infra_yav_secret_key': 'test_token',
        'aab_admin_tvm_secret_key': 'test_key',
        'aab_admin_tools_token_key': 'test_key',
        'aab_crypt_key_secret_key': 'test_key',
        'yp_resources': {
            'cpu_guarantee': 1000,  # mcores
            'cpu_limit': 2000,  # mcores
            'memory_guarantee': 4096,  # MB (at least 1536 for samogon infra)
            'memory_limit': 4096,  # MB
            'replicas': 1,
            'rootfs': 10 * 1024,  # MB
            'root_bandwidth_guarantee_megabytes_per_sec': 1,
            'root_bandwidth_limit_megabytes_per_sec': 2,
            'volumes': [
                {
                    'disk_quota_megabytes': 20 * 1024,
                    'mount_point': '/storage',
                    'storage_class': 'hdd',
                    'bandwidth_guarantee_megabytes_per_sec': 1,
                    'bandwidth_limit_megabytes_per_sec': 2,
                },
            ],
            'network_macro': '_ANTIADBNETS_',
        },
        'dcs': ['SAS'],

        'pg_db_host': 'sas-8pocbpki21xrqsld.db.yandex.net',
        'pg_db_port': '6432',
        'pg_db_name': 'aab_admin_yandex_team_ru_db_dev_2',

        'ENVIRONMENT_TYPE': 'TESTING',
        'HOST_DOMAIN': 'develop.antiblock.yandex.ru',
        'WEBMASTER_API_URL': 'https://webmaster3-internal.test.in.yandex.net',
        'INFRA_API_URL': 'https://infra-api-test.yandex-team.ru/v1/',
    },
}


def install_pg_cert(cert_path):
    root_cert = urllib2.urlopen('https://crls.yandex.net/allCAs.pem').read()
    with open(cert_path, 'w') as cert_file:
        cert_file.write(root_cert)


class AdminServer(object):
    __name__ = 'AdminServer'
    API_PORT = 8081

    def __init__(self, key, private_crypt_key, tvm_secret, db_pass, charts_token, sandbox_token, yt_token, tools_token, infra_token):
        self.config = CONFIG_BY_KEY[key]
        self.private_crypt_key = private_crypt_key
        self.tvm_secret = tvm_secret
        self.db_pass = db_pass
        self.charts_token = charts_token
        self.sandbox_token = sandbox_token
        self.yt_token = yt_token
        self.tools_token = tools_token
        self.infra_token = infra_token
        self.cert_path = '{srv}/root.crt'

    def balancer(self):
        return {
            'domain': 'api',
            'location': '/',
            'port': AdminServer.API_PORT,
            'balancing_options': {
                'connection_timeout': '200ms',
                'backend_timeout': '100s',
                'retries_count': 10,
                'balancer_retries_timeout': '310s',
                'keepalive_count': 10,
                'fail_on_5xx': False,
                'balancing_type': {
                    "mode": "weighted2",
                    "weighted2": {
                        "correction_params": {
                            "min_weight": 0.001,
                            "max_weight": 1.0,
                            "plus_diff_per_sec": 0.05,
                            "minus_diff_per_sec": 0.05,
                            "history_time": "20s",
                            "feedback_time": "10s",
                        },
                        "slow_reply_time": "5s",
                    },
                },
            }
        }

    def postinstall(self):
        install_pg_cert(self.substval(self.cert_path))

    def start(self):
        config = self.config
        env_vars = {}
        db_pass = self.db_pass
        db_host = config['pg_db_host']
        db_port = config['pg_db_port']
        db_name = config['pg_db_name']

        env_vars['DATABASE_URL'] = 'postgresql+psycopg2://antiadb:{db_pass}@{host}:{port}/{db_name}?' \
                                   'sslmode=verify-full&target_session_attrs=read-write&sslrootcert={root_cert}'.format(
            db_pass=db_pass,
            host=db_host,
            port=db_port,
            db_name=db_name,
            root_cert=self.substval(self.cert_path)
        )
        env_vars['PRIVATE_CRYPT_KEY'] = self.private_crypt_key
        env_vars['WEBMASTER_API_URL'] = config['WEBMASTER_API_URL']
        env_vars['INFRA_API_URL'] = config['INFRA_API_URL']
        env_vars['ENVIRONMENT_TYPE'] = config['ENVIRONMENT_TYPE']
        env_vars['HOST_DOMAIN'] = config['HOST_DOMAIN']
        env_vars['TVM_SECRET'] = self.tvm_secret
        env_vars['TOOLS_TOKEN'] = self.tools_token
        env_vars['CHARTS_TOKEN'] = self.charts_token
        env_vars['SANDBOX_TOKEN'] = self.sandbox_token
        env_vars['YT_TOKEN'] = self.yt_token
        env_vars['INFRA_TOKEN'] = self.infra_token
        env_vars['APP_BIND_STRING'] = '[::]:{port}'.format(port=AdminServer.API_PORT)
        self.create([self.find('configs_api_bin')], env=env_vars)

    def packages(self):
        return ['configs_api.tar.gz']


class DumpInstaller(object):
    __name__ = 'DumpInstaller'

    def __init__(self, config, yt_token, db_pass):
        self._config = config
        self.yt_token = yt_token
        self.db_pass = db_pass
        logging.info("DumpInstaller inited")

    def sysdeps(self):
        logging.info("DumpInstaller sysdeps")
        return {
            'deb': ['postgresql-client'],
        }

    def postinstall(self):
        logging.info("DumpInstaller start postinstall")
        yt_dump_path = '//home/antiadblock/configs_api/pg_db_dump/prod.dump'
        db_host = self._config['pg_db_host']
        db_port = self._config['pg_db_port']
        db_name = self._config['pg_db_name']
        env = os.environ.copy()
        env["PGPASSWORD"] = self.db_pass
        env["PGTARGETSESSIONATTRS"] = 'read-write'
        yt_host = json.loads(urllib2.urlopen('https://locke.yt.yandex-team.ru/hosts').read())[0]
        dump_request = urllib2.Request('https://{yt_host}/api/v3/read_file?path={yt_dump_path}'
                                       .format(yt_host=yt_host, yt_dump_path=yt_dump_path))
        dump_request.add_header('Authorization', 'OAuth {yt_token}'.format(yt_token=self.yt_token))
        pg_dump = urllib2.urlopen(dump_request).read()
        logging.info("DumpInstaller start read dump success len={}".format(len(pg_dump)))
        pg_restore_process = Popen(['pg_restore',
                                    '--clean',
                                    '--if-exists',
                                    '--no-owner',
                                    '--no-acl',
                                    '--single-transaction',
                                    '--schema=public',
                                    '--host={db_host}'.format(db_host=db_host),
                                    '--port={db_port}'.format(db_port=db_port),
                                    '--dbname={db_name}'.format(db_name=db_name),
                                    '--username=antiadb'], stdout=PIPE, stdin=PIPE, stderr=STDOUT, env=env)
        stdout, stderr = pg_restore_process.communicate(input=pg_dump)
        logging.warning("STDOUT {}".format(stdout))
        logging.warning("STDERR {}".format(stderr))


class PgMigrate(object):
    __name__ = 'PgMigrate'

    def __init__(self, config, db_pass):
        self._config = config
        self.db_pass = db_pass
        self.cert_path = '{srv}/root.crt'

    def packages(self):
        return ['migrations.tar.gz', 'pgmigrate.tar.gz']

    def postinstall(self):
        cert_path = self.substval(self.cert_path)
        install_pg_cert(cert_path)
        db_host = self._config['pg_db_host']
        db_port = self._config['pg_db_port']
        db_name = self._config['pg_db_name']
        env = os.environ.copy()
        env["PGPASSWORD"] = self.db_pass

        migration_target_dir = self.find_dir('migrations_tar_gz') + '/'
        conn_str = "dbname={db_name} host={db_host} port={db_port} " \
                   "user=antiadb target_session_attrs=read-write sslmode=verify-full sslrootcert={root_cert}" \
            .format(db_name=db_name,
                    db_host=db_host,
                    db_port=db_port,
                    root_cert=cert_path)
        pgmigrate_bin = self.find('pgmigrate')
        args = [
            pgmigrate_bin, "-t", "latest",
            "-d", migration_target_dir,
            "-c",
            conn_str,
            "-v",
            "migrate"
        ]
        logging.info("cmd args: %s" % ' '.join(args))
        subprocess.check_call(args, env=env, shell=False)


class Plugin(object):
    __name__ = 'aab_admin'

    def __init__(self, iface):
        self._iface = iface
        self._key = self._iface.key()
        self._config = CONFIG_BY_KEY[self._key]

    def conf(self):
        resources = self._config['yp_resources']
        return {
            'environment_settings': {
                'garbage_limit': '500 mb',
                'container_os': 'bionic',
            },
            'key': self._key,
            'abc_service_id': 1526,  # REQUIRED for yp-lite quota
            'yp_lite_allocation': {dc: resources for dc in self._config['dcs']}
        }

    def gencluster(self, iface):
        db_pass = iface.secrets[DB_YAV_SECRET_ID][self._config['db_yav_secret_key']]
        private_crypt_key = iface.secrets[AAB_CRYPT_KEY_SECRET_ID][self._config['aab_crypt_key_secret_key']]
        tvm_secret = iface.secrets[AAB_ADMIN_TVM_SECRET_ID][self._config['aab_admin_tvm_secret_key']]
        tools_token = iface.secrets[AAB_ADMIN_TOOLS_TOKEN_ID][self._config['aab_admin_tools_token_key']]
        charts_token = iface.secrets[CHARTS_YAV_SECRET_ID][self._config['charts_yav_secret_key']]
        sandbox_token = iface.secrets[SANDBOX_YAV_SECRET_ID][self._config['sandbox_yav_secret_key']]
        infra_token = iface.secrets[INFRA_YAV_SECRET_ID][self._config['infra_yav_secret_key']]
        yt_token = iface.samogon_token

        for host, info in iface.host_list():
            yield host, AdminServer(key=self._key, private_crypt_key=private_crypt_key, tvm_secret=tvm_secret,
                                    db_pass=db_pass, charts_token=charts_token, sandbox_token=sandbox_token,
                                    yt_token=yt_token, tools_token=tools_token, infra_token=infra_token)
        pg_host = iface.host_list()[0][0]
        logging.info("DumpInstaller is_prod={}".format(self._config['is_prod']))
        if not self._config['is_prod']:
            logging.info("yielding DumpInstaller host={}".format(pg_host))
            yield pg_host, DumpInstaller(self._config, yt_token, db_pass)
        yield pg_host, PgMigrate(config=self._config, db_pass=db_pass)

    def moderators(self):
        return ['evor', 'abc:antiadblock']

    def port_multiplier(self):
        return 100

    def is_test(self):
        return not self._config['is_prod']

    def deploy_scenario(self):
        scenario = {
            'PgMigrate': {
                'operating_degrade_level': 1,
                'stop_degrade_level': 0,
            },
            'AdminServer': {
                'operating_degrade_level': 0.3,
                'stop_degrade_level': 0.3,
                'dependencies': ('PgMigrate',)
            },
        }
        if not self._config['is_prod']:
            scenario.update({'DumpInstaller': {
                'operating_degrade_level': 1,
                'stop_degrade_level': 0,
            }})
            scenario['PgMigrate']['dependencies'] = ('DumpInstaller',)
        return scenario
