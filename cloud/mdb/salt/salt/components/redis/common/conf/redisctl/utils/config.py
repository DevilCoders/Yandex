#!/usr/bin/env python
# -*- coding: utf-8 -*-
import json

from .ctl_logging import log


HIDDEN_FIELDS = ['password', 'bgsave_rename', 'lastsave_rename', 'config_rename', 'bgrewriteaof_rename']


def read_json(fname):
    try:
        with open(fname) as f:
            data = json.load(f)
            assert isinstance(data, dict)
            return data
    except Exception as exc:
        log.error(exc, exc_info=True)


class Config(object):
    def __init__(self, args=None):
        """
        Order of config apply:
         - static
         - from args
         - dynamic (as it could be reloaded during redisctl run anyway, which could be long)
        :param args: args passed to redisctl on start
        :return: full config
        """
        self._dynamic_part = None
        self.config = {}
        self.CONFIG_STATIC = {}
        self.CONFIG_DYNAMIC = {
            'password': self.get_pass_from_file,
            'dbaas_conf': self.read_dbaas_config,
        }
        self.load_static(args)
        self.apply_args(args)
        self.reload()
        self.log_config()

    def load_static(self, args=None):
        path = self.config['args'].redis_data_path if args is None else args.redis_data_path
        self.CONFIG_STATIC = self.get_static_part(path)
        self.config.update(self.CONFIG_STATIC)

    def log_config(self):
        log_config = self.config.copy()
        for key in HIDDEN_FIELDS:
            if log_config.get(key):
                log_config[key] = '<REPLACED>'
        log.debug("current config: %s", log_config)

    @staticmethod
    def get_static_part(path):
        try:
            with open(path) as conf:
                config_static = json.load(conf)
        except Exception as exc:
            log.error('Failed to load the configuration file "%s": %s', path, repr(exc))
            config_static = {}
        return config_static

    def apply_args(self, args=None):
        if args is not None:
            self.config['args'] = args
        else:
            args = self.config['args']

        self.config['redis_configs'] = redis_configs = args.redis_conf_path
        self.config['sentinel_conf'] = args.sentinel_conf_path
        self.config['cluster_conf_path'] = args.cluster_conf_path
        self.config['dbaas_conf_path'] = args.dbaas_conf_path

        def get_option(opt):
            return _get_config_option_from_files(opt, config_path=redis_configs, index=2)

        self.config['bgsave_rename'] = get_option('rename-command BGSAVE')
        self.config['lastsave_rename'] = get_option('rename-command LASTSAVE')
        self.config['config_rename'] = get_option('rename-command CONFIG')
        self.config['shutdown_rename'] = get_option('rename-command SHUTDOWN')
        self.config['bgrewriteaof_rename'] = get_option('rename-command BGREWRITEAOF')
        self.config['restarts'] = args.restarts or self.config.get('restarts')
        self.config['timeout'] = args.timeout or self.config.get('timeout')
        self.config['sleep'] = args.sleep or self.config.get('sleep')
        self.config['bgsave_wait'] = args.bgsave_wait or self.config.get('bgsave_wait', 30 * 60)
        self.config['file'] = args.file or self.config.get('file')
        self.config['disk_limit'] = args.disk_limit or self.config.get('disk_limit')
        self.config['aof_limit'] = args.aof_limit or self.config.get('aof_limit')
        self.config['dry_run'] = args.dry_run or self.config.get('dry_run')
        self.config['force'] = args.force or self.config.get('force')
        self.config['options'] = args.options.split(',') if args.options else []

    def reload(self, options=None):
        options = self.CONFIG_DYNAMIC if options is None else options
        dynamic_config = {option: self.CONFIG_DYNAMIC[option]() for option in options}
        for k, v in dynamic_config.items():
            self.config[k] = v
        if self._dynamic_part is not None and dynamic_config and self._dynamic_part != dynamic_config:
            log_config = dynamic_config.copy()
            for key in HIDDEN_FIELDS:
                if log_config.get(key):
                    log_config[key] = '<REPLACED>'
            log.debug("config part reloaded: %s", log_config)
        self._dynamic_part = dynamic_config

    def get_redisctl_options(self, *options):
        if not set(options).issubset(set(self.config.keys())):
            raise RuntimeError(
                "requested options {} can't be received from config keys {}".format(options, self.config.keys())
            )
        dynamic_options = [option for option in options if option in self.CONFIG_DYNAMIC]
        self.reload(options=dynamic_options)
        values = [self.config.get(option) for option in options]
        return values[0] if len(values) == 1 else values

    def get_pass_from_file(self):
        password = None
        if 'redispass_files' not in self.CONFIG_STATIC:
            raise RuntimeError("no redispass files in config: {}".format(self.CONFIG_STATIC))
        files = self.CONFIG_STATIC['redispass_files']
        for pass_file in files:
            try:
                with open(pass_file) as redispass:
                    password = json.load(redispass)['password']
            except:
                pass
        if password:
            return password
        raise RuntimeError("password in all files is empty: {}".format(files))

    def read_dbaas_config(self):
        return read_json(self.config['dbaas_conf_path'])

    def __str__(self):
        return str(self.config)


def _get_config_option_from_connection(conn, config_cmd, pattern="*"):
    option, value = conn.execute_command('{} GET'.format(config_cmd), pattern)
    return value


def get_sentinel_config_option(option, config_path='/etc/redis/sentinel.conf', index=1):
    return _get_config_option_from_files(option, config_path=(config_path,), index=index)


def get_sharded_config_lines(config_path='/etc/redis/cluster.conf'):
    with open(config_path) as fobj:
        return fobj.readlines()


def _get_config_option_from_files(option, config_path=('/etc/redis/redis.conf', '/etc/redis/redis-main.conf'), index=1):
    """
    Get full value of option from config file.
    We expect config generated by our salt component here.
    """
    string_start = '{option} '.format(option=option)
    try:
        for path in config_path:
            with open(path) as config:
                for line in config:
                    if line.lower().startswith(string_start.lower()):
                        stripped = line.split(' ')[index].strip()
                        # Workaround for quotted config options
                        # (created by config rewrite command)
                        if stripped.startswith('"') and stripped.endswith('"'):
                            return stripped[1:-1]
                        return stripped
    except Exception as exc:
        log.error('Unable to get %s from %s: %s', option, config_path, repr(exc))


def _set_config_option(conn, config_cmd, option, value):
    res = conn.execute_command('{} SET'.format(config_cmd), option, value)
    return res, res == 'OK'
