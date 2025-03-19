#!/usr/bin/env python
# -*- coding: utf-8 -*-

import re
import os
import ConfigParser
import socket
import random
import string
import argparse
from cassandra.cluster import Cluster
from cassandra.auth import PlainTextAuthProvider


def set_cqlshrc(username, password):
    cqlshrc = "/root/.cassandra/cqlshrc"
    with open(cqlshrc, 'w+') as f:
        f.write("[authentication]\n")
        user_str = "username = %s\n" % (username)
        f.write(user_str)
        pass_str = "password = %s\n" % (password)
        f.write(pass_str)
        f.write("[connection]\n")
        hoststr = "hostname = %s\n" % (socket.gethostname())
        f.write(hoststr)
        f.write("port = 9042\n")


def random_generator(size=6, chars=string.ascii_uppercase + string.digits):
    return ''.join(random.choice(chars) for x in range(size))


def update_cqlshrc(filename):
    for line in open(filename):
        line = line.rstrip()
        match = re.search("^(\w+)\s+(\w+)\s*(\w+)?$", line)
        if match:
            if (match.groups()[0] == "cassandra"):
                print "Updating cqlshrc."
                set_cqlshrc(match.groups()[0], match.groups()[1])
            else:
                if (match.groups()[2] == "super"):
                    set_cqlshrc(match.groups()[0], match.groups()[1])
                    return


def get_file(path, user, secfile):
    cmdstr = 'scp -q -S ssh-noquiet -o "User=%s" -o "BatchMode=yes" -o "StrictHostKeyChecking=no" -o "UserKnownHostsFile=/dev/null" "secdist.yandex.net:/repo/projects/%s" "%s"' % (user, path, secfile)
    os.system(cmdstr)


def file_name():
    hostname = socket.gethostname().rsplit('.')
    hostname.reverse()
    hostname[-1] = re.sub("[0-9]+[a-z]?", "", hostname[-1])
    path = "/".join(hostname)
    return "ugc/common/cassandra/%s" % path


def apply_grants(filename):
    for line in open(filename):
        line = line.rstrip()
        match = re.search("^(\w+)\s+(\w+)\s+(\w.*)$", line)
        if match and match.groups()[2] != "super":
            if check_user_exists(match.groups()[0]):
                grants = match.groups()[2].rsplit(',')
                for el in grants:
                    if el == "S":
                        el = "SELECT"
                    if el == "D":
                        el = "DROP"
                    if el == "A":
                        el = "ALTER"
                    if el == "M":
                        el = "MODIFY"
                    if el == "C":
                        el = "CREATE"
                    query = "GRANT {} ON KEYSPACE {} TO {}".format(el, match.groups()[1], match.groups()[0])
                    print "Executing: " + query
                    session.execute(query)
            else:
                print "User " + match.groups()[0] + " does not exist but secfile has grants."


def change_cass_pass(filename):
    for line in open(filename):
        line = line.rstrip()
        match = re.search("^(\w+)\s+(\w+)$", line)
        if match:
            if (match.groups()[0] == "cassandra"):
                print "Changing cassandra superuser password."
                query = "ALTER USER cassandra WITH PASSWORD {!r};".format(match.groups()[1])
                session.execute(query)


def get_users(filename):
    users = []
    for line in open(filename):
        dic = {}
        line = line.rstrip()
        match = re.search("^(\w+)\s+(\w+)\s*(\w*)$", line)
        if match:
            if (match.groups()[0] != "cassandra" and match.groups()[2] != "super"):
                dic["username"] = match.groups()[0]
                dic["password"] = match.groups()[1]
                users.append(dic)
    return users


def get_superusers(filename):
    users = []
    for line in open(filename):
        dic = {}
        line = line.rstrip()
        match = re.search("^(\w+)\s+(\w+)\s+(\w.*)$", line)
        if match:
            if (match.groups()[2] == "super"):
                dic["username"] = match.groups()[0]
                dic["password"] = match.groups()[1]
                dic["super"] = match.groups()[2]

                users.append(dic)
    return users


def create_superusers(users):
    query = "LIST USERS"
    rows = session.execute(query)
    loc_users = []
    for row in rows:
        if row.super is True:
            loc_users.append(row.name)
    for user in range(len(users)):
        username = users[user]["username"]
        password = users[user]["password"]
        if check_user_exists(username):
            if username not in loc_users:
                print "Making " + username + " a superuser."
                query = "ALTER USER {} WITH PASSWORD {!r} SUPERUSER".format(username, password)
                session.execute(query)
        else:
            print "Creating SUPERUSER: " + username
            session.execute("CREATE USER {!s} WITH PASSWORD {!r} SUPERUSER".format(username, password))


def check_user_exists(username):
    rows = session.execute("LIST USERS")
    users = []
    for user in rows:
        users.append(user.name)
    if username in users:
        return True
    else:
        return False


def remove_users(users):
    loc_users = []
    new_users = []
    rows = session.execute("LIST USERS")
    for row in rows:
        if row.super is False:
            loc_users.append(row.name)
    for user in range(len(users)):
        new_users.append(users[user]["username"])
    for user in loc_users:
        if user not in new_users and user != "cassandra":
            print "Dropping user: {}".format(user)
            session.execute("DROP user {}".format(user))


def create_user(users):
    for user in range(len(users)):
        username = users[user]["username"]
        password = users[user]["password"]
        if check_user_exists(username):
            pass
        else:
            print "Creating user: " + username
            session.execute("CREATE USER {!s} WITH PASSWORD {!r}".format(username, password))


def parse_args():
    parser = argparse.ArgumentParser(description='Create, remove and apply grants to cassandra users.')
    parser.add_argument("-u", "--user", dest="user", required=False, 
                        help="Secdist username")
    #parser.add_argument("--host", dest="host", default=socket.gethostname(),
    #                    required=False, help="Cassandra fqdn host to connect to")
    parser.add_argument("-c", "--cache", dest="cache", required=False,
                        action="store_true", help="Use local cached file.")

    results = parser.parse_args()
    if not results.user:
            try:
                results.user = os.environ["LC_USER"]
            except KeyError:
                print "LC_USER not defined, use [-u|--user] option"
                sys.exit(0)
    return results


def create_cass_session(username, password, hostname):
    global session
    auth_provider = PlainTextAuthProvider(
            username=username, password=password)
    cluster = Cluster([hostname], auth_provider=auth_provider)
    session = cluster.connect()


def list_system_users():
            rows = session.execute("LIST USERS")
            for row in rows:
                print row


def parse_cqlshrc(cqlshrc):

    def config_section_map(section):
        dict1 = {}
        options = config.options(section)
        for option in options:
            try:
                dict1[option] = config.get(section, option)
            except:
                print "exception on %s!" % option
                dict1[option] = None
        return dict1

    config = ConfigParser.ConfigParser()
    config.read(cqlshrc)
    username = config_section_map("authentication")['username']
    password = config_section_map("authentication")['password']

    return username, password


def main():
    cqlshrc = "/root/.cassandra/cqlshrc"
    secfile = "/var/cache/cassandra/grants.cql"
    hostname = socket.gethostname()
    args = parse_args()
    file_str = file_name()
    if args.cache:
        if not secfile:
            print "Cache file is missing. Downloading file from secdist."
            get_file(file_str, args.user, secfile)
        else:
            print "Using cached file."
    else:
        print "Downloading file from secdist."
        get_file(file_str, args.user, secfile)
    update_cqlshrc(secfile)

    cass_user, cass_pass = parse_cqlshrc(cqlshrc)
    if cass_user != "cassandra":
        try:
            create_cass_session(cass_user, cass_user, hostname)
            super_users = get_superusers(secfile)
            create_superusers(super_users)
            change_cass_pass(secfile)
        except:
            create_cass_session("cassandra", "cassandra", hostname)
            super_users = get_superusers(secfile)
            create_superusers(super_users)
            create_cass_session(cass_user, cass_user, hostname)
            change_cass_pass(secfile)

    else:
        create_cass_session(cass_user, cass_user, hostname)

    users = get_users(secfile)
    create_user(users)
    remove_users(users)
    apply_grants(secfile)

main()
