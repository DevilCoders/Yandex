#!/usr/bin/env python
# -*- coding: utf-8 -*-
"""
MySQL user management commands for MDB
"""

import logging

from contextlib import closing

# For arcadia tests, populate __opts__ and __salt__ variables
__opts__ = {}
__salt__ = {}
__pillar__ = {}


CONNECTION_DEFAULT_FILE = '/home/mysql/.my.cnf'


log = logging.getLogger(__name__)


def __virtual__():
    return True


def user_grant_proxy_to(name, proxy_user=''):
    """
    Make user proxy of another
    """
    host = '%'
    full_name = '{}@{}'.format(name, host)
    ret = {
        'name': full_name,
        'result': None,
        'comment': 'proxy grants exists',
        'changes': {},
    }

    import MySQLdb

    with closing(MySQLdb.connect(read_default_file=CONNECTION_DEFAULT_FILE, db='mysql')) as conn:
        cur = conn.cursor(MySQLdb.cursors.DictCursor)

        err = None
        try:
            sql = 'SELECT User, Host, Proxied_user FROM mysql.proxies_priv where User = %(user)s and host =  %(host)s'
            args = {'user': name, 'host': host}
            cur.execute(sql, args)
            res = cur.fetchone()

            proxy_to_revoke = ''
            # no proxy to revoke
            if not res and not proxy_user:
                ret['result'] = True
                return ret

            if res:
                proxy_to_revoke = res['Proxied_user']
                if proxy_to_revoke == proxy_user:
                    ret['result'] = True
                    return ret

            if __opts__['test']:
                ret['result'] = None
                msg = 'changed' if proxy_to_revoke else 'given'
                ret['changes'] = {
                    full_name: 'proxy grants have to be {0}. Proxy user to be set: {1}'.format(msg, proxy_user)
                }
                return ret

            if proxy_to_revoke:
                sql = 'REVOKE PROXY ON `{proxy_to_revoke}`@`{host}` from `{name}`@`{host}`; FLUSH PRIVILEGES;'.format(
                    proxy_to_revoke=proxy_to_revoke, name=name, host=host
                )
                cur.execute(sql)

            if proxy_user:
                sql = 'GRANT PROXY ON `{proxy_user}`@`{host}` TO `{name}`@`{host}`; FLUSH PRIVILEGES;'.format(
                    name=name, proxy_user=proxy_user, host=host
                )
                cur.execute(sql)
        except MySQLdb.OperationalError as exc:
            err = 'Failed to execute {0}: due to {1}: "{2}"'.format(sql, exc.args[0], exc.args[1])
            log.error(err)
        if err:
            ret['result'] = False
            ret['comment'] = err
        else:
            ret['result'] = True
            ret['changes'] = {full_name: 'proxy grants were given. Current proxy user: {0}'.format(proxy_user)}
        return ret


def grant_proxy_grants(name, host, connection_default_file):
    """
    Ensure user has grant proxy privilege
    """
    ret = {
        'name': name,
        'result': True,
        'comment': [],
        'changes': {},
    }

    import MySQLdb

    user = '{}@{}'.format(name, host)

    with closing(MySQLdb.connect(read_default_file=connection_default_file, db='mysql')) as conn:
        cur = conn.cursor(MySQLdb.cursors.DictCursor)
        sql = 'SELECT 1 from mysql.proxies_priv where user = %(user)s and host = %(host)s'
        args = {'user': name, 'host': host}
        cur.execute(sql, args)
        grants_exist = cur.fetchone()

        if grants_exist:
            return ret

        if __opts__['test']:
            ret['result'] = None
            ret['changes'] = {user: 'proxy grants will be given'}
            return ret

        err = None
        try:
            sql = "INSERT INTO mysql.proxies_priv values (%(host)s, %(user)s,'','','1','root@', current_timestamp()); flush privileges;"
            args = {'user': name, 'host': host}
            cur.execute(sql, args)
        except MySQLdb.OperationalError as exc:
            err = 'Failed to execute {0} due to {1}: "{2}"'.format(sql, exc.args[0], exc.args[1])
            log.error(err)
        if err:
            ret['result'] = False
            ret['comment'] = err
        else:
            ret['result'] = True
            ret['changes'] = {user: 'proxy grants given'}
        return ret


def manage_user_lock(name, lock, host='%'):
    """
    Lock or unlock user
    """
    ret = {
        'name': name,
        'result': True,
        'comment': [],
        'changes': {},
    }

    import MySQLdb

    user = '{}@{}'.format(name, host)
    target_lock = 'Y' if lock else 'N'

    with closing(MySQLdb.connect(read_default_file=CONNECTION_DEFAULT_FILE, db='mysql')) as conn:
        cur = conn.cursor(MySQLdb.cursors.DictCursor)
        sql = "select account_locked from mysql.user where user = %(user)s and host = %(host)s"
        args = {'user': name, 'host': host}
        cur.execute(sql, args)
        res = cur.fetchone()

        if not res:
            ret['comment'] = 'user does not exist'
            return ret

        if res['account_locked'] == target_lock:
            ret['comment'] = '{} already {}'.format(user, 'LOCKED' if lock else 'UNLOCKED')
            return ret

        if __opts__['test']:
            ret['result'] = None
            ret['changes'] = {user: 'will be {}'.format('LOCKED' if lock else 'UNLOCKED')}
            return ret

        err = None
        try:
            sql = 'ALTER USER %(user)s@%(host)s ACCOUNT {lock_mode}'.format(lock_mode='LOCK' if lock else 'UNLOCK')
            args = {'user': name, 'host': host}
            cur.execute(sql, args)
        except MySQLdb.OperationalError as exc:
            err = 'Failed to execute {0} due to {1}: "{2}"'.format(sql, exc.args[0], exc.args[1])
            log.error(err)
        if err:
            ret['result'] = False
            ret['comment'] = err
        else:
            ret['result'] = True
            ret['changes'] = {user: 'was {}'.format('LOCKED' if lock else 'UNLOCKED')}
        return ret
