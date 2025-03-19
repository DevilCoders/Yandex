#!/usr/bin/env python
"""
Compute Image helper
"""

import argparse
import json
import os

from retry import retry
import requests

from vault_client.instances import Production as VaultClient
import yc_common.config as yc_common_config
from yc_common.clients.compute import ComputeClient


ENV_PREPROD = 'preprod'
ENV_PROD = 'prod'
ENVS = [ENV_PREPROD, ENV_PROD]
ENDPOINTS = {
    ENV_PREPROD: 'https://iaas.private-api.cloud-preprod.yandex.net',
    ENV_PROD: 'https://iaas.private-api.cloud.yandex.net',
}
TOKENS = {
    ENV_PREPROD: 'ver-01dw7q01bvx67b9k5yy75z37n2',
    ENV_PROD: 'ver-01e0cznph20v60bwd4ebpyq534'
}
metadata_token_url = 'http://169.254.169.254/computeMetadata/v1/instance/service-accounts/default/token'


def get_token_from_metadata():
    try:
        response = requests.get(metadata_token_url, headers={'Metadata-Flavor': 'Google'})
        response.raise_for_status()
        return response.json()['access_token']
    except Exception as exception:
        raise Exception(f'Could not get IAM token from {metadata_token_url}. Exception: {exception}')


def _get_client(env, token=None):
    if env not in ENVS:
        if not token:
            raise RuntimeError(f'Not found {env} in {ENVS}')
    if not token:
        token = VaultClient().get_version(TOKENS[env])['value']['token']

    url = ENDPOINTS[env]

    # Init empty config to use default values
    yc_common_config.load(['/dev/null'])

    return ComputeClient(
        '{base}/compute/external'.format(base=url), iam_token=token)


# workaround to avoid crashing for periodical SSL errors
@retry(tries=5, delay=10)
def wait_operation_with_retries(client, operation, wait_timeout=1200):
    print(f'Waiting operation {operation["id"]}.')
    client.wait_operation(operation, wait_timeout=wait_timeout)
    print(f'Operation {operation["id"]} completed.')


def get_client_and_variables(args):
    variables = {}
    with open(args.variables, 'r') as fp:
        variables = json.load(fp)
    iam_token = os.environ.get('IAM_TOKEN')
    if not iam_token and args.iam_token_file_path:
        iam_token = open(args.iam_token_file_path).read().strip()
    if not iam_token and args.token_from_metadata:
        iam_token = get_token_from_metadata()
    client = _get_client(variables.get('env', 'prod'), token=iam_token)
    return client, variables


def get_image_id_from_manifest(args):
    with open(args.manifest, 'r') as fp:
        manifest = json.load(fp)
    build = manifest.get('builds', [])[-1]
    if build is None:
        print('Not found any images in manifest')
        return
    image_id = build['artifact_id']
    return image_id


def update(args):
    client, variables = get_client_and_variables(args)

    image_id = args.image_id or get_image_id_from_manifest(args)
    operations = []

    pool_size = int(variables.get('pool_size', 5))
    if pool_size > 0:
        operation = client.update_disk_pooling(
            image_id=image_id, disk_count=pool_size, type_id='network-ssd')
        operations.append(operation)
        print(f'Updating disk pooling. Image id: {image_id}\n'
              f'Operation id: {operation["id"]}')

        operation = client.update_disk_pooling(
            image_id=image_id, disk_count=pool_size, type_id='network-hdd')
        print(f'Creating nbs pool on network-ssd for new image.\n'
              f'Operation id: {operation["id"]}')
        operations.append(operation)

    for operation in operations:
        wait_operation_with_retries(client, operation)


def create(args):
    """
    Recreate image and overwrite productId and create NBS pooling for new image
    """
    client, variables = get_client_and_variables(args)

    image_id = get_image_id_from_manifest(args)

    token_from_metadata = None
    if args.token_from_metadata:
        token_from_metadata = get_token_from_metadata()
    client = _get_client(variables.get('env', 'prod'), token=token_from_metadata)

    # Packer doesn't rewrites manifest.json on every run
    # insteadof it, packer adds run to builds list into existed manifest.
    # So, we will be use only latest run for creating image and pool
    image = client.get_image(image_id)
    operation = client.create_image(
        folder_id=variables['folder_id'],
        name=image['name'].replace('dataproc-', 'data-proc-'),
        description=image['description'],
        family=image['family'],
        labels=image['labels'],
        image_id=image_id,
        product_ids=[variables['product_id']],
        override_product_ids=True,
    )
    new_image_id = operation.metadata.image_id
    print(f'Creating new image. Image id: {new_image_id}')
    wait_operation_with_retries(client, operation, wait_timeout=600)

    image = client.get_image(new_image_id)
    print(f'New image {new_image_id} created\n'
          f'Attibutes {image}')

    operation = client.delete_image(image_id)

    args.image_id = new_image_id
    update(args)

    print(f'Deleting intermediate image with wrong productId. Image id: {image_id}\n'
          f'Operation id: {operation["id"]}')

    wait_operation_with_retries(client, operation)


COMMANDS = {
    'create': create,
    'update': update,
}


def _main():
    parser = argparse.ArgumentParser()
    parser.add_argument(
        'action', type=str, help='Operation', choices=COMMANDS.keys())
    parser.add_argument('--variables', type=str, help='packer variables file path')
    parser.add_argument('--token-from-metadata', action='store_true', help='get token from metadata')
    parser.add_argument('--iam-token-file-path', type=str, help='path to file containing IAM token')
    parser.add_argument('--manifest', type=str, help='packer manifest file path',
                        default='yandex-builder-manifest.json')
    parser.add_argument('--image-id', type=str, help='id of image to update pooling')
    args = parser.parse_args()

    COMMANDS[args.action](args)


if __name__ == '__main__':
    _main()
