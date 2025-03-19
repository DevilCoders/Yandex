#!/usr/bin/env python

import argparse
import grpc
import logging
import pwd
import os
import subprocess
import sys
import uuid
import yaml

from enum import Enum
from yandex.cloud.priv.infra.tpmagent.v1 import tpm_agent_pb2, tpm_agent_service_pb2_grpc


class CreateKeyPolicy(Enum):
    PER_HOST = 1
    PER_SERVICE_ACCOUNT = 2
    PER_ROLE = 3

    def __str__(self):
        return self.name

    @staticmethod
    def parse(s):
        try:
            return CreateKeyPolicy[s]
        except KeyError:
            raise ValueError()


DEFAULT_CA_PATH = '/etc/ssl/certs/ca-certificates.crt'
DEFAULT_CONFIG_FILE = '/etc/yc/token-agent/config.yaml'
DEFAULT_FORCE_CREATE = False
DEFAULT_KEY_POLICY = CreateKeyPolicy.PER_SERVICE_ACCOUNT
DEFAULT_ROLES_PATH = 'roles.d'
DEFAULT_GROUP_ROLES_PATH = 'groups.d'
DEFAULT_TOKEN_AGENT_PATH = '/var/run/yc/token-agent/socket'
PASSWORD_FILE_PATH = '/var/lib/yc/token-agent/key'

KEY_CACHE = {}

logger = logging.getLogger(__name__)


def make_path(path):
    if not os.path.exists(path):
        os.makedirs(path)
        logger.info('Created path "%s"', path)
    else:
        logger.debug('Path "%s" already exists', path)


def read_password():
    if os.path.exists(PASSWORD_FILE_PATH):
        logger.info('Reading key password from "%s"', PASSWORD_FILE_PATH)
        with open(PASSWORD_FILE_PATH, 'rt') as f:
            return f.read()

    logger.debug('Key password file not found, creating new one')
    new_password = uuid.uuid4().hex
    with open(PASSWORD_FILE_PATH, 'wt') as f:
        f.write(new_password)
    os.chmod(PASSWORD_FILE_PATH, 0o400)
    pw = pwd.getpwnam('yc-token-agent')
    os.chown(PASSWORD_FILE_PATH, pw.pw_uid, pw.pw_gid)
    logger.info('New key password saved to "%s"', PASSWORD_FILE_PATH)
    return new_password


def read_root_certificates(ca_file_path):
    with open(ca_file_path, 'rb') as f:
        certs = f.read()
    return certs


def open_channel(endpoint):
    url = 'unix://' + endpoint.get('host', DEFAULT_TOKEN_AGENT_PATH)
    tls = endpoint.get('useTls', False)
    if tls:
        ca_file_path = endpoint.get('ca_file_path', DEFAULT_CA_PATH)
        credentials = grpc.ssl_channel_credentials(root_certificates=read_root_certificates(ca_file_path))
        return grpc.secure_channel(url, credentials)
    else:
        return grpc.insecure_channel(url)


def get_cache_key(role, sa_id, policy):
    if policy == CreateKeyPolicy.PER_HOST:
        return "HOST"
    if policy == CreateKeyPolicy.PER_SERVICE_ACCOUNT:
        return sa_id
    if policy == CreateKeyPolicy.PER_ROLE:
        return role
    return None


def get_pub_key_for_handle(endpoint, role, key_handle):
    request_id = str(uuid.uuid4())
    logger.debug('Reading public key for role "%s" at "%s" with handle %s, request-id "%s"',
                 role, endpoint, key_handle, request_id)
    metadata = [
        ('x-request-id', request_id),
    ]

    with open_channel(endpoint) as channel:
        stub = tpm_agent_service_pb2_grpc.TpmAgentStub(channel)
        request = tpm_agent_pb2.ReadPublicRequest(handle=long(key_handle))
        try:
            response = stub.ReadPublic(request, metadata=metadata)
        except grpc.RpcError as e:
            logger.debug("GRPC error loading public key for handle %s, code %s",
                         key_handle, e.code())
            return None

    logger.info('\t=> Public key: "%s"', response.pub)
    return response.pub


def test_sign(endpoint, key_handle, key_password):
    request_id = str(uuid.uuid4())
    logger.debug('Checking TPM key password at "%s" with handle %s, request-id "%s"',
                 endpoint, key_handle, request_id)
    metadata = [
        ('x-request-id', request_id),
    ]

    with open_channel(endpoint) as channel:
        stub = tpm_agent_service_pb2_grpc.TpmAgentStub(channel)
        scheme = tpm_agent_pb2.SigScheme(hash=tpm_agent_pb2.Hash.SHA256, alg=tpm_agent_pb2.Alg.RSASSA)
        digest = '*' * 32
        request = tpm_agent_pb2.SignRequest(handle=long(key_handle), password=key_password, scheme=scheme, digest=digest)
        try:
            stub.Sign(request, metadata=metadata)
        except grpc.RpcError as e:
            logger.debug("GRPC error testing sign for handle %s, code %s",
                         key_handle, e.code())
            return False

    logger.info('\t=> OK')
    return True


def create_pub_key_and_handle(endpoint, key_password, role, sa_id, policy):
    cache_key = get_cache_key(role, sa_id, policy)
    logger.debug('cache key %s', cache_key)
    global KEY_CACHE
    cached = KEY_CACHE.get(cache_key, None)
    if cached:
        logger.debug('Response from cache')
        return cached

    request_id = str(uuid.uuid4())
    logger.debug('Making new key for role "%s" at "%s" with request-id "%s"', role, endpoint, request_id)
    metadata = [
        ('x-request-id', request_id),
    ]

    with open_channel(endpoint) as channel:
        stub = tpm_agent_service_pb2_grpc.TpmAgentStub(channel)
        request = tpm_agent_pb2.CreateRequest(hierarchy=tpm_agent_pb2.Hierarchy.OWNER, password=key_password)
        try:
            response = stub.Create(request, metadata=metadata)
        except grpc.RpcError as e:
            raise RuntimeError("Failed to create key. Make sure TPM agent is initialized and running.", e)

    logger.info('\t=> Key handle: "%s", public key "%s"', response.handle, response.pub)
    KEY_CACHE[cache_key] = response
    return response


def read_existing_key_handle(roles_path, role, sa_id):
    path = roles_path + '/' + role + '.yaml'
    if not os.path.exists(path):
        logger.info('Config file "%s" for role "%s" does not exist', path, role)
        return (None, None)

    with open(path, 'r') as config_file:
        config = yaml.safe_load(config_file)

    existing_key_handle = config.get("keyHandle")
    existing_key_id = config.get("keyId")
    existing_sa_id = config.get("serviceAccountId")

    if not existing_key_handle:
        logger.debug('Config file "%s" for role "%s" has no key handle', path, role)
        return (None, None)

    if existing_sa_id != sa_id:
        logger.debug('Config file "%s" for role "%s" has service account %s, expected %s',
                     path, role, existing_sa_id, sa_id)
        return (None, None)

    logger.info('Trying to reuse key handle %s, id "%s" for role "%s" from "%s"',
                existing_key_handle, existing_key_id, role, path)
    return (existing_key_id, existing_key_handle)


def write_config(roles_path, role, config):
    path = roles_path + '/' + role + '.yaml'
    make_path(os.path.dirname(path))
    with open(path, 'w') as config_file:
        yaml.dump(config, config_file)

    logger.info('Role "%s" config saved in "%s"', role, path)


def write_key_id_sa_id(roles_path, role, sa_id, key_handle):
    config = {
        'keyHandle': str(key_handle),
        'keyId': '',
        'serviceAccountId': sa_id,
    }
    write_config(roles_path, role, config)


def write_static_token(roles_path, role, static_token):
    config = {
        'token': static_token,
    }
    write_config(roles_path, role, config)


def bind_keys(roles_path, bind_keys_path):
    if bind_keys_path == '-':
        bind_keys = yaml.safe_load(sys.stdin)
    else:
        with open(bind_keys_path) as file_handle:
            bind_keys = yaml.safe_load(file_handle)

    logger.debug('Binging keys:\n%s', bind_keys)
    for role, data in bind_keys["tokenAgentKeys"].items():
        key_id = data['keyId']
        role_config = {}
        with open(roles_path + '/' + role + '.yaml', 'r') as config_file:
            role_config = yaml.safe_load(config_file)
        role_config['keyId'] = key_id
        with open(roles_path + '/' + role + '.yaml', 'w') as config_file:
            yaml.dump(role_config, config_file)

        logger.info('Key id "%s" for role "%s" saved in "%s"', key_id, role, roles_path)


def init_static(static_token_file, roles_path, roles):
    if '-' == static_token_file:
        static_token = sys.stdin.readline().strip()
    else:
        with open(static_token_file) as token_file:
            static_token = token_file.readline().strip()

    for role in roles:
        write_static_token(roles_path, role, static_token)


def create_keys(tpm_agent_endpoint, roles_path, roles, policy, force_create):
    token_agent_keys = dict()
    key_password = read_password()

    for role_sa_pair in roles:
        (role, sa_id) = role_sa_pair.split(":")
        pub_key = None
        if not force_create:
            (key_id, key_handle) = read_existing_key_handle(roles_path, role, sa_id)
            if key_handle and test_sign(tpm_agent_endpoint, key_handle, key_password):
                pub_key = get_pub_key_for_handle(tpm_agent_endpoint, role, key_handle)
                if pub_key and key_id:
                    logger.info('Role "%s" already initialized with key id %s', role, key_id)
                    continue

        # force create of existing key is not suitable
        if not pub_key:
            pub_key_and_handle = create_pub_key_and_handle(tpm_agent_endpoint, key_password, role, sa_id, policy)
            pub_key = pub_key_and_handle.pub
            key_handle = pub_key_and_handle.handle

        token_agent_keys[role] = {
            'serviceAccountId': sa_id,
            'publicKey': pub_key,
        }
        write_key_id_sa_id(roles_path, role, sa_id, key_handle)

    return yaml.dump({'tokenAgentKeys': token_agent_keys}, default_style='"', width=8192)


def restart_token_agent():
    ret_code = subprocess.call(['systemctl', 'restart', 'yc-token-agent.service'])
    if ret_code:
        logger.error('Failed to restart yc-token-agent: %s (%s)', os.strerror(ret_code), ret_code)
    else:
        logger.debug('yc-token-agent restarted')


def log_level_from_verbose(verbose):
    if verbose > 2:
        return logging.DEBUG

    if verbose > 1:
        return logging.INFO

    if verbose > 0:
        return logging.WARNING

    return logging.ERROR


def parse_args():
    parser = argparse.ArgumentParser()
    parser.add_argument('-b', '--bind-keys',
                        help='Bind public keys from yaml file; use "-" for stdin')
    parser.add_argument('-c', '--config',
                        help='Config file', default=DEFAULT_CONFIG_FILE)
    parser.add_argument('-f', '--force',
                        help='Never reuse exsisting TPM keys', action='store_true', default=DEFAULT_FORCE_CREATE)
    parser.add_argument('-g', '--group',
                        help='Apply changes to groups', action='store_true', default=False)
    parser.add_argument('-k', '--key-policy', type=CreateKeyPolicy.parse, choices=list(CreateKeyPolicy),
                        help='Key creation policy', default=DEFAULT_KEY_POLICY)
    parser.add_argument('-s', '--static-token',
                        help='Init with static token from file; use "-" for stdin')
    parser.add_argument('-v', '--verbose',
                        help='Verbose output', action='count', default=0)
    parser.add_argument('roles', nargs='*',
                        help='Roles to initialize')

    return parser.parse_args()


def main():
    args = parse_args()

    logger.setLevel(log_level_from_verbose(args.verbose))
    logger.addHandler(logging.StreamHandler())

    config = {}
    with open(args.config) as config_file:
        config = yaml.safe_load(config_file)

    if args.group:
        roles_path = config.get('groupRolesPath', DEFAULT_GROUP_ROLES_PATH)
    else:
        roles_path = config.get('rolesPath', DEFAULT_ROLES_PATH)

    if not os.path.isabs(roles_path):
        roles_path = os.path.join(os.path.dirname(args.config), roles_path)
    make_path(roles_path)

    if args.bind_keys:
        bind_keys(roles_path, args.bind_keys)
        restart_token_agent()
    elif args.static_token:
        init_static(args.static_token, roles_path, args.roles)
        restart_token_agent()
    else:
        tpm_agent_endpoint = config.get('tpmAgentEndpoint', {})
        if not tpm_agent_endpoint:
            token_agent_socket = config.get('listenUnixSocket')
            if token_agent_socket:
                tpm_agent_endpoint = {
                    'host': token_agent_socket.get('path', DEFAULT_TOKEN_AGENT_PATH),
                }
        print(create_keys(tpm_agent_endpoint, roles_path, args.roles, args.key_policy, args.force))


if __name__ == '__main__':
    main()
