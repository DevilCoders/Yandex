#!/skynet/python/bin/python
# coding: utf8
import json
import requests
import subprocess

from argparse import ArgumentParser

class EActions(object):
    MAKE_AND_UPLOAD = 'make_and_upload'
    PATCH = 'patch'
    CHOICES = [MAKE_AND_UPLOAD, PATCH]


def build_so_files():
    subprocess.check_call(['./build.sh'])


def tar_bin_utils():
    subprocess.check_call('tar -czvf __gencfg_bin_utils__.tar.gz core/*.so utils/binary/**', shell=True)
    subprocess.check_call(['mv', '__gencfg_bin_utils__.tar.gz', 'RESOURCE'])


def upload_bin_utils():
    subprocess.check_call([
        'ya', 'upload', '--sandbox', '--ttl=inf', '--owner=GENCFG', '--type=GENCFG_BIN_UTILS', 'RESOURCE'
    ])


def get_resource_info(resource_id):
    data = requests.get('https://sandbox.yandex-team.ru/api/v1.0/resource/{}'.format(resource_id)).json()

    try:
        rbtorrent = data['skynet_id']
        task_id = data['task']['id']
        rtype = data['type']
    except KeyError as e:
        print(data.keys(), e, data)
        raise

    return rbtorrent, task_id, rtype


def patch_sandbox_resources_file(resource_id, rbtorrent, task_id, rtype):
    with open('sandbox.resources') as rfile:
        data = json.load(rfile)

    data['binutils']['resource'] = resource_id
    data['binutils']['rbtorrent'] = rbtorrent
    data['binutils']['task'] = task_id
    data['binutils']['type'] = rtype

    with open('sandbox.resources', 'w') as wfile:
        json.dump(data, wfile, indent=4)


def parse_cmd():
    parser = ArgumentParser(description='Script to make GENCFG_BIN_UTILS and upload to sandbox.')
    parser.add_argument('-a', '--action', type=str, default='make_and_upload', choices=EActions.CHOICES,
                        help='Action with bin utils.')
    parser.add_argument('-r', '--resource_id', type=int, default=None, help='Resource id to patch.')

    options = parser.parse_args()

    if options.action == EActions.PATCH and options.resource_id is None:
        raise ValueError('Action `patch` need option --resource_id')

    return options


def main():
    options = parse_cmd()

    if options.action == EActions.MAKE_AND_UPLOAD:
        print('Build *.so files')
        build_so_files()

        print('Make bin utils archive')
        tar_bin_utils()

        print('Upload bin utils to sandbox')
        upload_bin_utils()

    elif options.action == EActions.PATCH:
        print('Get resource info')
        rbtorrent, task_id, rtype = get_resource_info(options.resource_id)

        print('Patch sandbox.resources file')
        patch_sandbox_resources_file(options.resource_id, rbtorrent, task_id, rtype)

    print('Done.')


if __name__ == '__main__':
    main()
