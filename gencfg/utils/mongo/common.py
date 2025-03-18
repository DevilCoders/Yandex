import os
import sys
import datetime
import collections
import json

import bson
import zlib
import msgpack

sys.path.append(os.path.abspath('.'))
import gencfg
import ujson
import core
import core.svnapi as svn


def recurse_fix_json_types(obj):
    if isinstance(obj, (dict, collections.OrderedDict)):
        result = {}
        for k, v in obj.iteritems():
            result[k] = recurse_fix_json_types(v)
        return result
    elif isinstance(obj, list):
        return [recurse_fix_json_types(x) for x in obj]
    elif isinstance(obj, tuple):
        return tuple([recurse_fix_json_types(x) for x in obj])
    elif isinstance(obj, core.card.types.ByteSize):
        return obj.value
    elif isinstance(obj, core.igroups.IGroup):
        return obj.card.name
    elif isinstance(obj, core.card.types.Date):
        return str(obj)
    elif isinstance(obj, bson.Binary):
        return None
    else:
        return obj


def binary(data):
    return bson.Binary(zlib.compress(msgpack.dumps(data)))


def de_binary(data):
    return msgpack.loads(zlib.decompress(data))


def binary2(data):
    return bson.Binary(zlib.compress(ujson.dumps(data, indent=4), 9))


def get_current_commit(path):
    for line in svn.SvnRepository(os.path.join(path, 'db')).command(['info']).stdout.splitlines():
        if line.startswith('Last Changed Rev'):
            return line.replace('Last Changed Rev:', '').strip()


def get_current_commit_creation_date(path):
    date = svn.SvnRepository(os.path.join(path, 'db')).svn_info_as_xml().find("entry/commit/date").text
    date = date.replace('<date>', '').replace('</date>', '').split('.')[0]
    return datetime.datetime.strptime(date, '%Y-%m-%dT%H:%M:%S')
