#!/usr/bin/env python

import argparse
import json
import logging
import os
import re
import subprocess
import sys
import xml.etree.ElementTree as ET

import ygetparam

DEFAULT_CACHE_DIR = 'data_loader_cache'
DEFAULT_CACHE_FILE = 'files_list'

ygetparam.expandExternalModules = False
MANIFEST_SUFFIX = ygetparam.MANIFEST_SUFFIX
# Do not use system etcd if any
for path in sys.path:
    mdl_path = os.path.join(path, ygetparam.MODULE_FOLDER)
    if os.path.exists(mdl_path):
        sys.path.insert(0, mdl_path)
        import etcd
        break
DEFAULT_ETCD_HOST = 'ygetparam.n.yandex-team.ru'
DEFAULT_ETCD_PORT = 80
MANIFEST = ygetparam.EXTERNAL_STORAGE_CONF['etcd']['manifest_fmt']

COMMENT_LEX = '#'
SVN_ROOT = 'svn+ssh://arcadia.yandex.ru/arc/trunk/arcadia'
SVN_LOG_CMD_FMT = "svn log -l 1 --xml {}/{}"
SVN_CAT_CMD_FMT = "svn cat -r {} {}/{}"

LOG_LEVELS = ('debug', 'info', 'warning', 'error', 'critical')
LOG_MSG_FORMAT = "%(asctime)s {}".format(ygetparam.LOG_MSG_FORMAT)
LOG_DEFAULT_LEVEL = logging.ERROR


# Exceptions
class DataLoadeException(Exception):
    '''Base exception class.'''
    pass


class IncorrectFileNameException(DataLoadeException):
    pass


def Upload(instance, file_name):
    ''' str, str -> bool
    Check file for correctness (also remove formating if any)
    '''
    # File should be a valid JSON
    try:
        obj_name = MANIFEST.format(instance)
        manifest = ygetparam.LoadJSON(file_name)
        ygetparam.GetResolvedManifest(manifest, dict(), '')
        log.info("Upload manifest for {!r} instance.".format(instance))
        connect = etcd.Client(host=args.host, port=args.port, allow_redirect=False)
        connect.write(obj_name, json.dumps(manifest))
    except ValueError as err:
        log.warning("{} in {!r}".format(err, file_name))
        return False
    return True


def Update(file_name, rev):
    ''' str, str -> str|None
    Retrive new revision of a file from svn
    '''
    try:
        cmd = SVN_CAT_CMD_FMT.format(rev, SVN_ROOT, file_name).split()
        data = subprocess.check_output(cmd, stderr=subprocess.STDOUT)
    except subprocess.CalledProcessError as err:
        log.warning(err.output)
        return None
    f_name = os.path.join(CACHE_DATA_DIR, os.path.basename(file_name))
    with open(f_name, 'w') as fd:
        fd.write(data)
        log.info("{!r} bytes stored in {!r}".format(len(data), f_name))
    return f_name


def GetFilesList(src_file):
    ''' str -> list
    Read and return list of file name
    '''
    files = list()
    with open(src_file, 'r') as fd:
        for line in fd.readlines():
            line = line.strip()
            if not line.startswith(COMMENT_LEX):
                files.append(line)
    return files


def GetCachedRevs(files):
    ''' list -> dict(str:str, ...)
    Read cached revisions for each of listd files (if available)
    '''
    # Prepare cache
    if not os.path.exists(CACHE_DATA_DIR):
        os.makedirs(CACHE_DATA_DIR)
    open(CACHE_FILE, 'a').close()

    cache = dict()
    with open(CACHE_FILE, 'r') as fd:
        for line in fd.readlines():
            name, rev = line.split()
            if name in files:
                cache[name] = rev
    return cache


def GetInstanceName(file_name):
    ''' str -> str
    Extract service name from file name.
    '''
    m = re.match('(?P<instance_name>\w+)' + MANIFEST_SUFFIX, os.path.basename(file_name))
    if m:
        instance_name = m.group('instance_name')
    else:
        msg = "Unble to extract instance name from the file name. Use '-i' option."
        raise IncorrectFileNameException(msg)
    return instance_name


def GetRevision(file_name):
    ''' str -> str
    Return current revision of the given file
    '''
    cmd = SVN_LOG_CMD_FMT.format(SVN_ROOT, file_name).split()
    xml = subprocess.check_output(cmd, stderr=subprocess.STDOUT)
    root = ET.fromstring(xml)
    # XML exmaple:
    #       <?xml version="1.0" encoding="UTF-8"?>
    # root  <log>
    # [0]   <logentry
    #          revision="2007353">
    #       <author>vlegeza</author>
    #       <date>2015-12-14T10:37:55.072010Z</date>
    #       <msg>Port conflict fix</msg>
    #       </logentry>
    #       </log>
    revision = root[0].attrib['revision']
    return revision


if __name__ == '__main__':
    parser = argparse.ArgumentParser()
    parser.add_argument('file',
                        type=str,
                        help="manifest file (must be <service>{} [or use -i])".format(MANIFEST_SUFFIX))
    parser.add_argument('-l', '--log-file',
                        type=str,
                        default=None,
                        help='where to write log '
                        '[default: STDERR]')
    parser.add_argument('-c', '--cache-dir',
                        type=str,
                        default=DEFAULT_CACHE_DIR,
                        help='cache directory [default: %(default)s]')
    parser.add_argument('--log-level',
                        type=str,
                        choices=LOG_LEVELS,
                        default='error',
                        action='store',
                        help='set logging level [default: %(default)s]')
    parser.add_argument('--host',
                        type=str,
                        default=DEFAULT_ETCD_HOST,
                        help='etcd host [default: %(default)s]')
    parser.add_argument('--port',
                        type=int,
                        default=DEFAULT_ETCD_PORT,
                        help='etcd port [default: %(default)s]')
    parser.add_argument('--debug', action='store_true', help='Print Traceback on exception.')
    group = parser.add_mutually_exclusive_group()
    group.add_argument('-i', '--instance', type=str, nargs=1, help='instance(service) name.')
    group.add_argument('--from-file',
                       action='store_true',
                       help='read Arcadia paths(one per line) from specified manifest file. '
                       'Checkout new versions accordingly to data cache file '
                       'and load them into etcd storage. Also, updated content will be stored '
                       'localy in cache directory')
    args = parser.parse_args()

    log = logging.getLogger()
    if args.log_file:
        logHandler = logging.FileHandler(args.log_file)
    else:
        logHandler = logging.StreamHandler()
    if args.log_level:
        numeric_level = getattr(logging, args.log_level.upper())
        log.setLevel(numeric_level)
    else:
        log.setLevel(LOG_DEFAULT_LEVEL)
    logHandler.setFormatter(logging.Formatter(LOG_MSG_FORMAT))
    log.addHandler(logHandler)

    CACHE_DATA_DIR = args.cache_dir
    CACHE_FILE = os.path.join(CACHE_DATA_DIR, DEFAULT_CACHE_FILE)

    try:
        if args.from_file:
            files = GetFilesList(args.file)
            cache = GetCachedRevs(files)
            for item in files:
                try:
                    rev = GetRevision(item)
                    if item not in cache:
                        cache[item] = None
                    if cache[item] != rev:
                        log.info("Require update: {!r} from rev:{} to rev:{}".format(item, cache[item], rev))
                        cached_file = Update(item, rev)
                        if cached_file:
                            if Upload(GetInstanceName(cached_file), cached_file):
                                cache[item] = rev
                except subprocess.CalledProcessError as err:
                    for line in err.output.split('\n'):
                        if line.startswith('svn'):
                            log.warning(line)
            with open(CACHE_FILE, 'w') as fd:
                for item in sorted(cache):
                    fd.write("{} {}\n".format(item, cache[item]))
                log.info("Cache file {!r} updated with {} entries.".format(CACHE_FILE, len(cache)))
        elif args.instance:
            Upload(args.instance[0], args.file)
        else:
            Upload(GetInstanceName(args.file), args.file)
    except Exception as err:
        log.error(str(err))
        if args.debug:
            raise
        sys.exit(1)
