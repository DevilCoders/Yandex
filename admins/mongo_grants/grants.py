# -*- coding: utf-8 -*-
"""
Grants module
Load grants file from local copy
Parse grants file in memory object and merge common users
from all grants sections
"""
import os
import re
import copy
import json
from collections import namedtuple, defaultdict

import yaml
from helpers import CONSOLE, handle_error, dict2tuple


DEF_ADMIN_GRANTS = [{"db": "admin", "role": "userAdminAnyDatabase"},
                    {"db": "admin", "role": "readWriteAnyDatabase"},
                    {"db": "admin", "role": "dbAdminAnyDatabase"},
                    {"db": "admin", "role": "clusterAdmin"}]


class Grant(namedtuple('GrantDefinition', 'name db grants authdb')):
    "Grants object"
    def __new__(cls, name, db, grants, authdb=None):
        return super(Grant, cls).__new__(cls, name, db, grants, authdb)


class User(namedtuple('UserDefinition', 'name passwd root')):
    "User object"
    def __new__(cls, name, passwd, root=False):
        return super(User, cls).__new__(cls, name, passwd, root == 'root')


def parse_custom_text(grants_text, grants_object):
    """
    Parse custom grants defined as json text
    custom grants always defined in last section
    """
    CONSOLE.ok("Processing section 'custom-roles'")
    custom = yaml.load('\n'.join(grants_text or '{}'))
    CONSOLE.trace("Custom yaml data: {0}".format(
        json.dumps(custom, indent=2, sort_keys=True, default=str)))
    for key, value in custom.iteritems():
        if re.match(r'\s*(repl|mongodb)', key):
            key = 'mongodb'
        elif re.match(r'\s*(shard|mongos)', key):
            key = 'mongos'
        CONSOLE.debug("Custom roles in mode {0}".format(key))
        grants_object[key]["custom"] = value


def parse_user_text(text, users_object):
    """
    Parse line with user definition
    """
    user = [line.strip() for line in text.split()]

    if len(user) < 2:
        CONSOLE.debug("Skip line {0}".format(text))
        return

    if len(user) < 3 or user[2] != 'root':
        user.insert(2, False)

    user_tuple = User(*user)
    CONSOLE.info(user)

    user = users_object[user_tuple.name]
    if not user:
        user.update(user_tuple._asdict())
        user['roles'] = defaultdict(list)
    else:
        user['passwd'] = user_tuple.passwd


def parse_grants_text(text, grants_object, mode):
    """
    Parse line with grants definition
    """
    grants = Grant(*text.split(None, 3))
    CONSOLE.info(grants)
    if not grants_object[mode]['users'][grants.name]:
        grants_object[mode]['users'][grants.name]['roles'] = defaultdict(list)

    authdb = grants.authdb or grants.db
    grants_object[mode]['users'][grants.name]['roles'][authdb].extend(
        [{'role': r, 'db': grants.db} for r in grants.grants.split(',')]
    )


def parse_section_name(name, grants_object):
    """
    Parse section name and mode of this section
    """
    section, mode = [x.strip() for x in name[1:].split(']', 1)]
    mode = mode or 'common'
    CONSOLE.notice("Processing section '{1}.{0}'".format(section, mode))
    if re.match(r'\s*(repl|mongodb)', mode):
        mode = 'mongodb'
    elif re.match(r'\s*(shard|mongos)', mode):
        mode = 'mongos'

    if not section.startswith('grants') and not grants_object[mode].get(section):
        CONSOLE.debug("New section '{0}.{1}'".format(section, mode))
    return section, mode


@handle_error(msg="Fix grants source file and try again", exit_code=1)
def parse_grants(grants_raw):
    """
    Parse grants file, this function force well known grants file syntax.
    Skip unknown lines and allow optional fileds for user and grants
    """
    CONSOLE.info("Parse grants")
    grants = defaultdict(lambda: defaultdict(lambda: defaultdict(dict)))
    skip_section = False
    grants_raw_split = grants_raw.splitlines()

    # namedtuple with defautls value
    for _ in range(len(grants_raw_split)):
        line = grants_raw_split.pop(0).strip()
        CONSOLE.debug("Parse line: {0}".format(line))

        line = line.split('#', 1)[0].strip()
        if line.startswith('['):
            if re.match(r'\s*\[(users|grants)\]', line):
                skip_section = False
                sec, mod = parse_section_name(line, grants)
            elif line.startswith('[custom-roles]'):
                parse_custom_text(grants_raw_split, grants)
                break
            else:
                CONSOLE.warn("Unknown section {0}, Skip!".format(line))
                skip_section = True
                continue
        else:
            if not line or skip_section:
                CONSOLE.debug("Skip line for unknown section")
                continue
            if sec == 'grants':
                parse_grants_text(line, grants, mod)
            elif sec == 'users':
                parse_user_text(line, grants[mod]['users'])
            else:
                CONSOLE.error("Unknown section name: {0}, skip".format(sec))
                skip_section = True
                continue
    return grants


def dbs_with_special_marks(roles_object):
    """
    Collect db with special mark '__all__'
    """
    dbs = []
    for db_name in roles_object.keys():
        if '__all__' in [d['db'] for d in roles_object[db_name]]:
            dbs.append(db_name)
    return dbs


def expand_all(client, users):
    """Expand special mark '__all__' and generate grants for all databases except
    [config, local, test, admin]."""

    users = copy.deepcopy(users)
    for user, data in users.items():
        if not all((data.get('name'), data.get('passwd'), data.get("roles"))):
            CONSOLE.debug("Invalid user '{0}: {1}', skip.".format(user, data))
            continue

        user_roles = data["roles"]
        expanded_user_roles = defaultdict(set)

        dbs_with_special = dbs_with_special_marks(user_roles)
        if not dbs_with_special:
            continue

        CONSOLE.debug("Have special marks in authdbs '{0}'".format(dbs_with_special))
        # assume we have client authentificated on admin db here
        dbs = client.admin.command('listDatabases')['databases']
        dbs = set([d['name'] for d in dbs])
        dbs = dbs - {'config', 'local', 'test', 'admin'}

        # stages 1: expand roles
        for db_name in user_roles.keys():
            if db_name not in dbs_with_special:
                # add normal roles to result dict as set of tuples
                expanded_user_roles[db_name].update(
                    map(dict2tuple, user_roles[db_name])
                )
                continue  # go to next db_name

            CONSOLE.debug("Expand special in authdb '{0}' for dbs '{1}'".format(db_name, dbs))
            # if db_name not __all__, use it as authdb else authdb will de db_name
            # individually itself for every mongodb's databases
            authdb = db_name if db_name != '__all__' else None

            one_db_users_roles = user_roles[db_name]

            # iterate over roles within exists mark '__all__'
            for role in one_db_users_roles:
                if role["db"] != '__all__' and authdb:
                    # special mark found but authdb specified and current role
                    # with explicit db name, add is for authdb only
                    expanded_user_roles[authdb].add(dict2tuple(role))
                    continue

                if role["db"] != '__all__' and not authdb:
                    # authdb not specified and db is not mark
                    # add role in every db
                    for one_db in dbs:
                        expanded_user_roles[one_db].add(dict2tuple(role))
                    continue

                CONSOLE.debug("Expand special role `{0}' for user `{1}'".format(
                    role, user))
                for one_db in dbs:
                    new_role = (('db', one_db), ('role', role['role']))
                    expanded_user_roles[authdb or one_db].add(new_role)

        # stages 2: convert roles back to dict
        for db_name in expanded_user_roles:
            expanded_user_roles[db_name] = map(dict, expanded_user_roles[db_name])
        # update user roles and convert defaultdict to dict
        users[user]["roles"] = dict(expanded_user_roles)
        CONSOLE.debug("Expanded roles for '{0}':\n{1}".format(
            user, json.dumps(users[user]['roles'], indent=2, sort_keys=True, default=str)))
    return users


@handle_error(msg="Not found admin user", exit_code=1)
def merge_common_users(grants, mode):
    """
    Merge common users with users in specified mode
    """
    users_add = copy.deepcopy(grants['common']['users'])
    mode_users = copy.deepcopy(grants[mode]['users'])
    for user, data in mode_users.items():
        # if user defined only in grants
        if 'name' not in data:
            data['name'] = user
        if users_add.get(user):
            for db_name, roles in data['roles'].items():
                users_add[user]['roles'][db_name].extend(roles)
            # if user defined only in grants
            users_add[user].setdefault('name', user)
            if data.get('passwd'):
                users_add[user]['passwd'] = data['passwd']
        else:
            users_add[user] = data
    try:
        admin_names = [name for name in users_add if users_add[name].get('root')]
        for name in admin_names:
            if not users_add[name]['roles'].get('admin'):
                msg = "User '{0}' have not grants, but admin! fix."
                CONSOLE.debug(msg.format(name))
                CONSOLE.notice("Grant default roles for user '{0}': {1}".format(
                    name, json.dumps(DEF_ADMIN_GRANTS, indent=2, sort_keys=True, default=str)))
                users_add[name]['roles']['admin'] = DEF_ADMIN_GRANTS
        root = users_add[admin_names[0]]

        # iterate over users and delete malformed
        users_names = users_add.keys()
        for name in users_names:
            # delete users without password or roles
            if 'passwd' not in users_add[name]:
                CONSOLE.warn("User without passwod `{0}', i delete him!'".format(name))
                del users_add[name]
            elif not users_add[name].get('roles'):
                CONSOLE.warn("User without roles `{0}', i delete him!'".format(name))
                del users_add[name]
        return root, users_add
    except Exception:
        msg = "Current mode: {0}. For replica/shard only run with -r/-s option!"
        CONSOLE.warn(msg.format(mode))
        raise


def get_grants(local):
    "Read grans from local file or secdist"

    fname = local
    CONSOLE.ok("Read grants from local copy '{0}'".format(fname))
    with open(fname, 'r') as grants_file:
        grants = grants_file.read()
    if not local:
        os.remove(fname)
        CONSOLE.debug("remove temporarry grants file {0}".format(fname))
    grants = parse_grants(grants)
    return grants
