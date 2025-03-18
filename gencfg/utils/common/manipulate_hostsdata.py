#!/skynet/python/bin/python

"""Manipulate hosts data

Perform various actoins:
    - prepare an upload new resource
"""

import os
import sys

sys.path.append(os.path.abspath(os.path.join(os.path.dirname(__file__), '..', '..')))

import xmlrpclib
import json
import tempfile
import shutil

import api.copier

import gencfg
from core.db import CURDB
from core.argparse.parser import ArgumentParserExt
from gaux.aux_utils import wait_sandbox_task, request_sandbox
from gaux.aux_colortext import red_text
from gaux.aux_mongo import get_mongo_collection
import mmh3
import ujson


class EActions(object):
    UPLOAD = 'upload'  # upload new sandbox resource
    STATUS = 'status'  # status of current hosts_data
    CHECK = 'check'  # status of current hosts_data
    ALL = [UPLOAD, STATUS, CHECK]


def get_parser():
    parser = ArgumentParserExt(description='Perform various actions with hosts_data')
    parser.add_argument('-a', '--action', type=str, required=True,
                        choices=EActions.ALL,
                        help='Obligatory. Action to execute')
    parser.add_argument('-m', '--message', type=str, default=None,
                        help='Obligatory. Description of new sandbox task with uploaded resource')
    parser.add_argument('--switch-to', action='store_true', default=False,
                        help='Optional. Switch to uploaded hosts_data resource')
    parser.add_argument('--external-workdir', type=str, default=None,
                        help='Optional. Working directory of a parent task')

    return parser


def normalize(options):
    if options.action == EActions.UPLOAD:
        if options.message is None:
            raise Exception('You must specify <--message> option in action <{}>'.format(options.action))


def main_upload(options):
    if CURDB.version <= '2.2.40':
        print red_text('Can not upload: db version <{}> does not require hosts_data uploading'.format(CURDB.version.asstr()))
        return
    if not CURDB.hosts.is_hosts_data_modified():
        print red_text('File hosts_data is not modified (compare to current resource)')
        return

    # share current hosts_data
    copier = api.copier.Copier()
    if options.external_workdir:
        shutil.copy(os.path.join(CURDB.HDATA_DIR, 'hosts_data'), options.external_workdir)
        skynet_id = copier.create(['hosts_data'], cwd=options.external_workdir).resid()
    else:
        skynet_id = copier.create(['hosts_data'], cwd=CURDB.HDATA_DIR).resid()

    # create task
    data = json.dumps({
        'type': 'REMOTE_COPY_RESOURCE',
        'description': options.message,
        'owner': 'GENCFG_SANDBOX_IMPORTANT_TASKS',
        'important': True,
        'custom_fields': [
            {'name': 'resource_type', 'value': 'GENCFG_HOSTS_DATA'},
            {'name': 'resource_type_name', 'value': 'hosts_data'},
            {'name': 'remote_file_name', 'value': skynet_id},
            {'name': 'remote_file_protocol', 'value': 'skynet'},
        ],
    })
    task_id = request_sandbox('/task', data=data, method='POST')['id']

    # start task
    data = json.dumps({
        'comment': 'Start remote copy resource',
        'id' : [task_id],
    })
    request_sandbox('/batch/tasks/start', data=data, method='PUT')

    # get resource
    wait_sandbox_task(task_id)

    new_resource_id = request_sandbox('/resource?limit=1&type=GENCFG_HOSTS_DATA&task_id={}'.format(task_id))['items'][0]['id']

    # switch to resource
    print 'Uploaded resource <{}> in task <{}>'.format(new_resource_id, task_id)
    if options.switch_to:
        with open(CURDB.hosts.configfile, 'w') as f:
            f.write(json.dumps(dict(resource_id=new_resource_id)))
        os.remove(os.path.join(CURDB.HDATA_DIR, 'hosts_data'))
        print red_text('Switched to resource <{}> in hosts_data, commit required'.format(new_resource_id))


def main_status(options):
    print 'Status:'

    if CURDB.version <= '2.2.40':
        print '    Db version <{}> is less than <2.2.41>: used hosts_data from reporsitory'.format(CURDB.version.asstr())
        return

    # Do not use CURDB.hosts structures, this will cause automatic update
    hostsfile = os.path.join(CURDB.HDATA_DIR, 'hosts_data')
    configfile = os.path.join(CURDB.HDATA_DIR, 'hosts_data.url')  # file with sandbox resource id
    statusfile = os.path.join(CURDB.HDATA_DIR, 'hosts_data.status')

    new_resource_id = ujson.loads(open(configfile).read())['resource_id']
    if not os.path.exists(hostsfile):
        print '     File db/hardware_data/hosts_data does not exist: need switch None -> {}'.format(new_resource_id)
    elif not os.path.exists(statusfile):
        print '     File db/hardware_data/hosts_data.status does not exist: need switch None -> {}'.format(new_resource_id)
    else:
        jsoned = ujson.loads(open(statusfile).read())
        current_resource_id = jsoned['resource_id']
        current_resource_hash = jsoned['resource_hash']

        hosts_data_modified = (current_resource_hash != mmh3.hash(open(hostsfile).read()))
        if new_resource_id == current_resource_id:
            print '   Used resource: last release {}'.format(current_resource_id)
            if hosts_data_modified:
                print red_text('        Data is modified')
        else:
            print red_text('   Used resource: need switch {} -> {}'.format(current_resource_id, new_resource_id))
            if hosts_data_modified:
                print red_text('        Data is modified, can not switch on new version: {} -> {}'.format(current_resource_id, new_resource_id))


def main_check(options):
    # Do not use CURDB.hosts structures, this will cause automatic update
    hostsfile = os.path.join(CURDB.HDATA_DIR, 'hosts_data')
    statusfile = os.path.join(CURDB.HDATA_DIR, 'hosts_data.status')

    if not os.path.exists(hostsfile):
        return # not existed hosts data can not be corrupted

    jsoned = ujson.loads(open(statusfile).read())
    current_resource_hash = jsoned['resource_hash']

    hosts_data_modified = (current_resource_hash != mmh3.hash(open(hostsfile).read()))
    if hosts_data_modified:
        print red_text('Hosts data is modified! Should run this first: ./utils/common/manipulate_hostsdata.py -a upload -m "New hosts_data" --switch-to')
        exit(1)


def main(options):
    if options.action == EActions.UPLOAD:
        main_upload(options)
    elif options.action == EActions.STATUS:
        main_status(options)
    elif options.action == EActions.CHECK:
        main_check(options)
    else:
        raise Exception('Unknown action <{}>'.format(options.action))


if __name__ == '__main__':
    options = get_parser().parse_cmd()

    normalize(options)

    main(options)
