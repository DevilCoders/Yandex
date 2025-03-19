# -*- coding: utf-8 -*-
"""
MySQL migration functions
"""

import os
import logging
from subprocess import Popen, PIPE
import hashlib

from contextlib import closing

try:
    import six
except ImportError:
    from salt.ext import six

log = logging.getLogger(__name__)
connection_default_file = "/home/mysql/.my.cnf"

__opts__ = {}
__salt__ = {}


def check_schema(name):
    """
    Checks migration schema
    """
    ret = {
        'name': name,
        'result': None,
        'comment': '',
        'changes': {},
    }

    try:
        import MySQLdb
    except ImportError:
        ret['result'] = False
        ret['comment'] = 'Failed to apply migration: MySQLdb is not installed yet'
        return None

    with closing(MySQLdb.connect(read_default_file=connection_default_file, db='mysql')) as conn:
        cur = conn.cursor(MySQLdb.cursors.DictCursor)
        try:
            cur.execute('show tables like \'mdb_migration\'')
            if cur.fetchone():
                ret['result'] = True
                ret['changes'] = {}
                return ret
        except MySQLdb.OperationalError as exc:
            ret['result'] = False
            ret['changes'] = {name: "failed to check table \'mdb_migration\'"}
            ret['comment'] = 'MySQL Error {0}: {1}'.format(*exc.args)
            return ret

        if __opts__['test']:
            ret['result'] = None
            ret['changes'] = {name: 'Table \'mdb_migration\' have to be created'}
            return ret

        log.debug('Creating table \'mdb_migration\'')

        try:
            cur.execute('create table mdb_migration (file VARCHAR(256) primary key, md5 VARCHAR(256))')
            ret['result'] = True
            ret['changes'] = {name: 'Table \'mdb_migration\' was created'}
        except MySQLdb.OperationalError as exc:
            ret['result'] = False
            ret['changes'] = {name: "failed to create table \'mdb_migration\'"}
            ret['comment'] = 'MySQL Error {0}: {1}'.format(*exc.args)

    return ret


def apply_migration(name, migration):
    """
    Applies migration to MySQL
    """
    ret = {
        'name': name,
        'result': None,
        'comment': [],
        'changes': {},
    }

    try:
        import MySQLdb
    except ImportError:
        ret['result'] = False
        ret['comment'] = 'Failed to apply migration: MySQLdb is not installed yet'
        return None

    if not os.path.exists(migration) and __opts__['test']:
        ret['result'] = None
        ret['changes'] = {migration: 'will be applied for the first time'}
        return ret

    hash = _calc_md5(migration)

    with closing(MySQLdb.connect(read_default_file=connection_default_file, db='mysql')) as conn:
        cur = conn.cursor(MySQLdb.cursors.DictCursor)
        cur.execute('select md5 from mdb_migration where file = \'{0}\''.format(migration))
        result = cur.fetchone()

        apply_first_time = not result
        if not apply_first_time:
            # migration has been applied
            curr_hash = result['md5']
            if curr_hash == hash:
                ret['result'] = True
                ret['changes'] = {}
                return ret

        if __opts__['test']:
            ret['result'] = None
            if apply_first_time:
                ret['changes'] = {migration: 'will be applied for the first time'}
            else:
                ret['changes'] = {migration: 'has changed and will be re-applied'}
            return ret

        code, err = _execute_migration(migration)
        if code:
            ret['result'] = False
            ret['comment'] = 'error executing migration: {0}'.format(err)
            return ret

        _save_migration_info(cur, migration, hash, ret)

    return ret


def _save_migration_info(cur, migration, hash, ret):
    import MySQLdb

    query_template = """
    INSERT INTO mdb_migration (file, md5) values (\'{file}\', \'{hash}\')
    ON DUPLICATE KEY UPDATE md5=\'{hash}\';
    commit;"""

    try:
        cur.execute(query_template.format(file=migration, hash=hash))
        ret['result'] = True
        ret['changes'] = {migration: 'migration applied'}
    except MySQLdb.OperationalError as exc:
        err = 'MySQL Error {0}: {1}'.format(*exc.args)
        ret['result'] = False
        ret['changes'] = {migration: "failed to save migration info"}
        ret['comment'] = err


def _execute_migration(migration):
    log.info('running migration \'{0}\''.format(migration))
    process = Popen(['sudo', '-u', 'mysql', 'mysql'], stdin=PIPE, stderr=PIPE)
    cmd = _ensure_binary('source {0}'.format(migration))
    _, stderr = process.communicate(input=cmd)
    return process.poll(), _ensure_str(stderr)


def _calc_md5(fname):
    hash_md5 = hashlib.md5()
    with open(fname, "rb") as f:
        for chunk in iter(lambda: f.read(4096), b""):
            hash_md5.update(chunk)
    return hash_md5.hexdigest()


def _ensure_binary(s, encoding='utf-8', errors='strict'):
    """Coerce **s** to six.binary_type.
    For Python 2:
      - `unicode` -> encoded to `str`
      - `str` -> `str`
    For Python 3:
      - `str` -> encoded to `bytes`
      - `bytes` -> `bytes`
    NOTE: The function equals to six.ensure_binary that is not present in the salt version of six module.
    """
    if isinstance(s, six.binary_type):
        return s
    if isinstance(s, six.text_type):
        return s.encode(encoding, errors)
    raise TypeError("not expecting type '%s'" % type(s))


def _ensure_str(s, encoding='utf-8', errors='strict'):
    """Coerce *s* to `str`.
    For Python 2:
      - `unicode` -> encoded to `str`
      - `str` -> `str`
    For Python 3:
      - `str` -> `str`
      - `bytes` -> decoded to `str`
    NOTE: The function equals to six.ensure_str that is not present in the salt version of six module.
    """
    if type(s) is str:
        return s
    if six.PY2 and isinstance(s, six.text_type):
        return s.encode(encoding, errors)
    elif six.PY3 and isinstance(s, six.binary_type):
        return s.decode(encoding, errors)
    elif not isinstance(s, (six.text_type, six.binary_type)):
        raise TypeError("not expecting type '%s'" % type(s))
    return s
