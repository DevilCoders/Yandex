#!/usr/bin/env python3

# This script is intended to simplify initialization of terraforms with secrets
# Prerequsites:
#   - ya vault

import argparse
import os
import subprocess

from functools import cached_property

ROOT = os.path.dirname(os.path.realpath(__file__))


def parse_args():
    parser = argparse.ArgumentParser(description='Initialize terraform.')
    parser.add_argument(
        'group',
        choices=['datasphere', 'speechkit', 'translate', 'vision', 'infra'],
        help='target group'
        )
    parser.add_argument(
        'target',
        help='target name'
        )
    parser.add_argument(
        'environment',
        choices=['preprod', 'staging', 'prod'],
        help='environment to initialize in'
        )
    parser.add_argument(
        '-f',
        action='store_true',
        help='override secret files during the initialization'
        )
    return parser.parse_args()


class Target:
    def __init__(self, group, target, environment):
        self.__group = group
        self.__target = target
        self.__environment = environment

    @property
    def group(self):
        return self.__group

    @property
    def target(self):
        return self.__target

    @property
    def environment(self):
        return self.__environment

    @cached_property
    def path(self):
        return os.path.join(ROOT, 'targets', self.group, self.target, self.environment)

    @cached_property
    def parent_path(self):
        return os.path.join(ROOT, 'targets', self.group, self.target)


def write_artifact_file(path, content, force):
    if os.path.exists(path):
        if force:
            os.remove(path)
        else:
            raise RuntimeError("{} already exist".format(path))
    with open(path, "w") as f:
        f.write(content)


def get_secret(environment, key):
    if key == '':
        raise RuntimeError('Key should not be empty')

    yav_secrets = {
        "common":  "sec-01dhkfv32aarnbqszm9h2cse7x",
        "preprod": "sec-01f9a2xftdhrrz3a2h9wggy7kx",
        "staging": "sec-01djmfwpjn2a5hwzn7gcm2mmcz",
        "prod":    "sec-01djn2s8h6b657he0hy6hfhpvf"
    }
    result = subprocess.run([
        'ya', 'vault', 'get', 'version',
        yav_secrets[environment],
        '-o', key
        ], capture_output=True, text=True)
    if result.returncode != 0:
        print(result.stderr)
        print(result.stdout)
        raise RuntimeError("Could not retrieve secret")
    return result.stdout.strip('\n')

def get_ds_secret(environment, key):
    yav_secrets = {
        "preprod": "sec-01enq9bpyrtg1y4jg2c16q2mck",
        "staging": "sec-01epvx9ntze2zdtd01zzmtnh22",
        "prod":    "sec-01ent56gp5fbzvvknsteg5bd55"
    }
    result = subprocess.run([
        'ya', 'vault', 'get', 'version',
        yav_secrets[environment],
        '-o', key
        ], capture_output=True, text=True)
    if result.returncode != 0:
        print(result.stderr)
        print(result.stdout)
        raise RuntimeError("Could not retrieve secret")
    return result.stdout.strip('\n')


# TODO: Currently all environments contains the same secret_key
#       -> secret_key should be moved to common secret
def create_backend_config_file(target, force):
    access_key = "fHNdo3JRxGBaPymGbqZd"
    secret_key = get_secret(
        target.environment,
        'access-secret-key-terraform-maintainer'
        )

    config = '\n'.join([
        'access_key = "{}"'.format(access_key),
        'secret_key = "{}"'.format(secret_key),
        ''
        ])

    config_path = os.path.join(target.path, "backend.tfvars")
    write_artifact_file(config_path, config, force)
    return config_path


def init_terraform(target_path, backend_config_path):
    result = subprocess.run([
        'terraform',
        'init',
        '-backend-config={}'.format(backend_config_path)
        ],
        cwd=target_path)
    if result.returncode != 0:
        raise RuntimeError("Could not init terraform")


def get_deployer_key(target):
    deployer_key_file = os.path.join(target.parent_path, 'deployer_key.txt')

    if not os.path.isfile(deployer_key_file):
        raise RuntimeError('There are no deployer_key.txt file')

    with open(deployer_key_file) as f:
        return f.read().strip('\n')


def create_deployer_sa_key_file(target, force):
    deployer_key = get_deployer_key(target)

    service_key = get_secret(
        target.environment,
        deployer_key
        )

    sa_key_file_path = os.path.join(target.path, '.yc_sa_key')
    write_artifact_file(sa_key_file_path, service_key, force)
    return sa_key_file_path


def create_tfvar_file(target, force):
    yandex_token = get_secret(
        target.environment,
        'yav-token'
    )

    sa_key_file = create_deployer_sa_key_file(target, force)

    s3_access_key = get_ds_secret(
        target.environment,
        's3_access_key'
    )

    s3_secret_key = get_ds_secret(
        target.environment,
        's3_private_key'
    )

    tvm_secret = get_ds_secret(
        target.environment,
        'tvm_secret'
    )

    db_password = get_ds_secret(
        target.environment,
        'db_password'
    )

    config = '\n'.join([
        'yandex_token = "{}"'.format(yandex_token),
        'service_account_key_file = "{}"'.format(sa_key_file),
        's3_access_key = "{}"'.format(s3_access_key),
        's3_secret_key = "{}"'.format(s3_secret_key),
        'tvm_secret = "{}"'.format(tvm_secret),
        'db_password = "{}"'.format(db_password),
        ''
        ])

    terraform_tfvars = os.path.join(target.path, "terraform.tfvars")
    write_artifact_file(terraform_tfvars, config, force)
    return terraform_tfvars


def initialize_target(target, force):
    if not os.path.isdir(target.path):
        raise ValueError('target does not exist')

    backend_config_path = create_backend_config_file(target, force)
    init_terraform(target.path, backend_config_path)
    create_tfvar_file(target, force)


def main():
    args = parse_args()
    target = Target(args.group, args.target, args.environment)
    initialize_target(target, args.f)


if __name__ == "__main__":
    main()
