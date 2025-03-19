"""pingunoque config"""
import os
import socket
import argparse
import yaml

PARSER = argparse.ArgumentParser(description="Small keepalived-like daemon")
PARSER.add_argument('-c', '--config', help='Config path')
ARGS = PARSER.parse_args()

def load_config():
    """
    Load main config
    Try load default config file if user defined file not exists
    """

    conf_file = '/etc/yandex/pingunoque/config-default.yaml'
    if os.path.isfile('/etc/yandex/pingunoque/config.yaml'):
        conf_file = '/etc/yandex/pingunoque/config.yaml'
    if not os.path.isfile(conf_file):
        # dev
        conf_file = 'conf/config-default.yaml'

    if ARGS.config:
        conf_file = ARGS.config

    with open(conf_file) as _cf:
        config = yaml.load(_cf)
    return config

def validate_config(cfg, log):
    """Check daemon config and substitude smart defaults if posible"""
    proto_number = socket.getprotobyname(cfg['check_type'])
    log.debug("Resolved proto number: %s", proto_number)
    cfg['proto_number'] = proto_number

    if cfg['check_type'] == 'tcp':
        log.debug("Set check_port=%s", cfg.setdefault('check_port', 13))
        log.debug("Set tcp_timeout=%s seconds", cfg.setdefault("tcp_timeout", 1.0))
    elif cfg['check_type'] == 'icmp':
        log.debug("Set icmp_timeout=%s seconds", cfg.setdefault("icmp_timeout", 1.0))
        if cfg.get('check_port', None):
            log.debug("Reset check_port to 'None' for icmp check")
            cfg['check_port'] = None
    else:
        raise RuntimeError("Unknown check type")

    log.debug("Set check_retry=%s", cfg.setdefault("check_retry", 2))
    log.debug("Set dns_cache_time=%s", cfg.setdefault('dns_cache_time', 300))
    log.debug("Set dns_error_limit=%s", cfg.setdefault('dns_error_limit', 3))
    log.debug("Set unban_on_dns_errors=%s", cfg.setdefault("unban_on_dns_errors", True))
    log.debug("Set close_on_socket_error=%s", cfg.setdefault('close_on_socket_error', False))
    return cfg

def load_and_validate(log):
    """Load and validate pingunoque config"""
    cfg = validate_config(load_config(), log)
    return argparse.Namespace(**cfg)
