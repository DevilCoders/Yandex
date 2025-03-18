# coding: utf-8

import sys
import os

import hjson

from library.python.oauth import get_token

from tools.releaser.src.lib import files


def parse_config(config_path):
    try:
        with open(config_path, 'r') as config_file:
            return hjson.load(config_file)
    except IOError:
        pass
    except hjson.scanner.HjsonDecodeError:
        msg = ' '.join([
            'Can\'t parse config file "%s". ' % config_path,
            'Config must be in HJSON format, see http://hjson.org/'
        ])
        sys.exit(msg)


class QloudInstance:
    INT = 'int'
    EXT = 'ext'


QLOUD_URL_MAP = {
    QloudInstance.INT: 'https://qloud.yandex-team.ru',
    QloudInstance.EXT: 'https://qloud-ext.yandex-team.ru',
}


# регулярка, где матчится одна группа — номер текущей версии
SETUP_PY_VERSION_REGEX = 'version[\s_]+=\s?(?:\'|\")([\d\w\.-]+)(?:\'|\")'
PACKAGE_JSON_VERSION_REGEX = '"version"\s?:\s?"([\d\w\.-]+)"'
YA_MAKE_VERSION_REGEX = 'VERSION\(([\d\w\.-]+)\)'

VERSION_REGEXS = [
    SETUP_PY_VERSION_REGEX,
    PACKAGE_JSON_VERSION_REGEX,
    YA_MAKE_VERSION_REGEX,
]

SLUG = os.path.basename(os.getcwd())
DEFAULTS = {
    'oauth_token': None,
    'registry_host': 'registry.yandex.net',
    'docker_info_url': 'https://dockinfo.yandex-team.ru',
    'internal_ca_path': '/etc/ssl/certs/ca-certificates.crt',
    'remote': 'origin',
    'version_build_arg': False,
    'non_interactive': False,
    'image': SLUG,
    'pull_base_image': False,
    'pull_vcs': False,
    'dockerfile': 'Dockerfile',
    'buildfile': None,
    'sandbox': False,
    'sandbox_config': {},
    'qloudinst': QloudInstance.EXT,
    'project': 'tools',
    'application': SLUG,
    'applications': SLUG,
    'environment': 'testing',
    'target_state': None,
    'component': None,
    'components': None,
    'environment_dump_from': None,
    'standname': 'unstable',
    'domain_type': 'internal',
    'domain_tpl': None,
    'deploy_comment_format': 'Releasing version {version}, changelog:\n{changelog}',
    'deploy_hook': None,
    'version_files': {
        # filename: regex or None for all defaults
    },
    'experiments': [],
    'box': None,
    'stage': None,
    'deploy_units': None,
    'deploy': None,
    'deploy_draft': None,
    'tvm_client_id': 2023432,
    'logbroker_topic': '/intranet/intranet-awacs-access-logs',
}


class Config(object):

    QloudInstance = QloudInstance

    def __init__(self):
        self.options = None

    def ensure_options_loaded(self):
        if self.options is not None:
            return

        self.options = {}
        self.options.update(self.get_defaults())
        self.options.update(self.get_from_files())
        self.options.update(self.get_from_env_vars())

        if not self.options['oauth_token']:
            self.options['oauth_token'] = get_token(
                client_id='69bfc53a5e7144c281ccf88ca7e98598',
                client_secret='f2227248411f4aa1a62312dd0a265064',
            )

        if isinstance(self.options['version_files'], list):
            self.options['version_files'] = {version_file: None for version_file in self.options['version_files']}
        elif not self.options['version_files']:
            version_file = self.options.get('version_file_path') or files.find_first_existing_file(['setup.py', 'src/setup.py', 'package.json'])
            self.options['version_files'] = {version_file: None}

    def get_defaults(self):
        return dict(DEFAULTS)

    def get_from_files(self):
        file_options = {}
        for config_path in self.get_config_paths():
            config_options = parse_config(config_path)
            if config_options:
                file_options.update(config_options)
        return file_options

    def get_from_env_vars(self):
        env_var_options = {}
        for env_var, value in os.environ.items():
            if not env_var.startswith('RELEASER_'):
                continue
            option_name = env_var[len('RELEASER_'):].lower()
            env_var_options[option_name] = value

        # обратная совместимость, консистентнее будет RELEASER_OAUTH_TOKEN
        oauth_token = os.environ.get('QLOUD_API_OAUTH_TOKEN')
        if oauth_token:
            env_var_options['oauth_token'] = oauth_token
        return env_var_options

    @staticmethod
    def get_config_paths():
        expected_paths = (
            # сюда удобно класть на CI
            ('RELEASER_GLOBAL_CONFIG', '/etc/release.hjson'),
            # сюда удобно класть на ноуте
            # (oauth_token, qloud_project: tools, qloud_instance: ext, ...),
            # чтобы писать один раз, а не везде
            (
                'RELEASER_USER_CONFIG',
                os.path.join(
                    os.environ.get('HOME', ''),
                    '.release.hjson',
                )
            ),
            # настройки текущего проекта: app: at-back, image, ...
            ('RELEASER_PROJECT_CONFIG', './.release.hjson'),
            # необязательный локальный конфиг, чтобы временно переопределить
            # проектный, когда нужно несколько сборок из одного репозитория
            ('RELEASER_LOCAL_CONFIG', None)
        )
        paths = []
        for env_var, default_path in expected_paths:
            path = os.environ.get(env_var, default_path)
            if path:
                paths.append(path)
        return paths

    def __getattr__(self, item):
        self.ensure_options_loaded()
        key = item.lower()
        if key in self.options:
            return self.options[key]
        else:
            raise AttributeError(
                'Config variable %s not found among %s' % (
                    item,
                    list(self.options)
                )
            )

    def __contains__(self, item):
        self.ensure_options_loaded()
        key = item.lower()
        return key in self.options

    def __getitem__(self, item):
        return self.__getattr__(item)

    def is_experiment_enabled(self, name):
        return name in self.EXPERIMENTS

    def get_command_option(self, cmd_name, option_name):
        return self.options.get(cmd_name + '.' + option_name)

    @staticmethod
    def get_qloud_url(qloud_instance):
        return QLOUD_URL_MAP[qloud_instance]

    def __repr__(self):
        return repr(self.options)


cfg = Config()
