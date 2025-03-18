#!/skynet/python/bin/python

import os
import sys

sys.path.append(os.path.abspath(os.path.join(os.path.dirname(__file__), '..', '..')))
import gaux.aux_utils

import xmlrpclib
import api.copier
from argparse import ArgumentParser
from tempfile import mkdtemp

import gencfg

SANDBOX_URL = 'https://sandbox.yandex-team.ru/sandbox/xmlrpc'


def parse_cmd():
    parser = ArgumentParser(description="Downloads resource (file) from sandbox via skynet")
    parser.add_argument("-t", "--type", type=str, dest="resource_type",
                        required=True,
                        help="Obligatory. Resource type")
    parser.add_argument("-d", "--dest", type=str, dest="dest",
                        required=True,
                        help="Obligatory. Destination path")
    parser.add_argument("-s", "--state", type=str, dest="state",
                        default="READY",
                        help="Optional. Resource state")
    parser.add_argument("-a", "--arch", dest="arch",
                        choices=['linux', 'freebsd', 'any'],
                        default=os.uname()[0].lower(),
                        help="Optional. Architecture")

    if len(sys.argv) == 1:
        sys.argv.append('-h')
    options = parser.parse_args()

    if options.arch == 'any':
        options.arch = None
    options.limit = 1

    return options


if __name__ == '__main__':
    options = parse_cmd()

    request = {k: v for k, v in options.__dict__.items()
               if k not in ['dest']}
    if request['arch'] == 'any':
        del request['arch']

    proxy = xmlrpclib.ServerProxy(SANDBOX_URL)
    res_id = proxy.listResources(request)[0]['skynet_id']

    copier = api.copier.Copier()
    resource_files = copier.handle(res_id).list(network=api.copier.Network.Backbone).wait().files()
    if len(resource_files) == 0:
        raise Exception('Resource not found')
    if len(resource_files) > 1:
        raise Exception('Multiple file resources download is not implemented')
    resource_file = resource_files[0]
    if resource_file['type'] != 'file':
        raise Exception('Invalid resource type %s' % resource_file['type'])

    base_dir = os.path.dirname(os.path.abspath(options.dest))
    # copier cannot download resource to specific filename,
    # so let's make temp dir and then rename downloaded file
    tmp_dir = mkdtemp(prefix='.tmp', dir=base_dir)
    os.chmod(tmp_dir, 0o777)
    try:
        copier.get(res_id, tmp_dir, user=gaux.aux_utils.getlogin(), network=api.copier.Network.Backbone).wait()
        src_file = os.path.join(tmp_dir, resource_file['name'])
        os.rename(src_file, options.dest)
    finally:
        for fname in os.listdir(tmp_dir):
            os.unlink(os.path.join(tmp_dir, fname))
        os.rmdir(tmp_dir)
