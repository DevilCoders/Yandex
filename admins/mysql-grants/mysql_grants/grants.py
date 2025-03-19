import requests
import re
import os, sys
import socket
import StringIO
import git
import MySQLdb
import subprocess
import netaddr
import yaml
from hashlib import sha1
from utils import *

class Grants():

    def __init__(self, legacy, verbose, dryrun):

        self.legacy = legacy
        self.verbose = verbose
        self.me = socket.gethostname()
        self.grants = {}
        self.users = {}
        self.hosts = {}
        # hosts in raw like _CONTENTTNETS_ - unresolved macroses, groups and hostnames
        self.local_users = {}
        self.conn = None
        self.path = None
        self.repo_local_path = None
        self.repo_remote_path = None
        self.existing_users = None
        self.tools = utils(self.verbose)
        self.ex_grants = {}
        #b_users - users prepared by build_users - dict which contains usersnames in mysql notation, passwords and grants
        self.b_users = {}
        self.sql = []
        self.dryrun = dryrun
        self.expand = True
        self.failed = 0
        self.cache_path = '/var/cache/mysql-grants-4/'


    def build_users(self):

        for user in self.grants:
            for host in self.grants[user]:
                # get grants dict for host
                pre_grants =  self.grants[user][host]['grants']
                post_grants = {}

                # we must process theese grants and edit it

                # process all destinations
                for dst in pre_grants:
                    # process * -> *.*
                    # grant = host_grants.get(key)
                    # get raw grant
                    grant = pre_grants.get(dst)
                    # split var like db.table to convert it into `db`.`table`
                    db_table = dst.split(".")
                    # check case when  db_table is var like '*'
                    if len(db_table) > 1:
                        try:
                            m_db_table = '`' + db_table[0] + '`' + '.' + '`' + db_table[1] + '`'
                            post_grants.update({m_db_table : grant})
                        except:
                            self.msg('Failed to process grant: ' + grant.__str__() + ' user: ' + user + ' dst: ' + dst.__str__(), bcolors.FAILRED)
                            pass
                            # grant.update( { nk : g } )
                    else:
                        try:
                            # process vars like db (means db.*)
                            if dst == '*':
                                m_db_table = dst + '.*'
                            else:
                                m_db_table = '`' + dst + '`' + '.*'

                            post_grants.update( { m_db_table : grant } )

                        except:
                            self.msg('Failed to process grant: ' + grant.__str__() + ' user: ' + user + ' dst: ' + dst.__str__(), bcolors.FAILRED)
                            pass

                # update processed grant for current destination

                self.grants[user][host].update( { 'grants' : post_grants } )

                for addr in self.grants[user][host]['ip']:
                    # for abstract hosts like procedure_hosts
                    if re.match('^[a-z][a-z0-9\.\-]+', addr):
                        mhost = addr

                    elif re.match('^[a-z0-9]+$', addr):
                        mhost = addr

                    else:
                        mhost = self.tools.net_to_mhost(addr)

                    if mhost != None:
                        muser = '\'' + user + '\'' + '@' + '\'' + mhost + '\''

                    else:
                        if addr == 'localhost' or addr == '%':
                            muser = '\'' + user + '\'' + '@' + '\'' + addr + '\''
                        else:
                            self.msg('Address ' + addr + " doesn't look like correct address", bcolors.WARNYELLOW)
                            muser = None
                            # print self.grants[user][host]

                    try:
                        passwd = self.mypassword(self.users[user])
                    except:
                        self.msg('Failed to process user: ' + user, bcolors.FAILRED)

                    if passwd == None:
                        passwd = ''

                    if muser is not None:
                        if not muser in self.b_users:
                            # userslist.update({ muser : { 'grants' : grants[user][host]['grants'], 'pwd' : passwd } })
                            self.b_users.update( { muser: {'grants': self.grants[user][host]['grants'], 'pwd': passwd } } )

    def expand_grants(self):
        addrs = []
        for grant in self.grants:
            for host in self.grants[grant]:
                for macro in self.grants[grant][host]['ip']:
                    # detect raw mysql host definition
                    if re.match('^[a-fA-F0-9\.\:%]+$', macro):
                    #if re.match('^[a-z0-9\.\:%]+$', macro):
                        addrs.append(macro)
                    # detect word definition like localhost or other thing that
                    # not needed to resolve
                    elif re.match(r'^[a-z_\-0-9]+$', macro):
                        addrs.append(macro)
                    # detect fqdn definition
                    elif re.match(r'^[a-z0-9\.\-]+$', macro):
                        # exception for grants that not
                        addr = self.tools.resolve_host(macro)
                        if addr != None:
                            addrs.extend(addr)
                        # process hosts like procedure_host which are abstract surfaces and has no addresses
                        else:
                            addrs.append(macro)


                    # detect network macroses like _CONTENTNETS_
                    elif re.match(r'^[A-Z0-9_]+$', macro):
                        m = self.tools.expand_macros(macro)
                        if m != None:
                            addrs.extend(m)
                    # process conductor group
                    elif re.match(r'^%[a-z0-9_\-]+$', macro):
                        tmp = self.tools.group2hosts(macro)
                        if tmp != None:
                            for h in tmp:
                                addrs.extend(self.tools.resolve_host(h))
                    else:
                        addrs.append(macro)

                if len(addrs) > 0:
                    self.grants[grant][host].update({'ip': addrs})
                    addrs = []


    def get_path(self):
        if self.legacy:
            self.path = self.tools.get_grants_repo() + "/" +  self.tools.get_grants_filename()
        else:
            # use config if exists(for more usability)
            if os.path.exists('/etc/mysql-configurator/grants.conf'):
                self.msg("Use values from /etc/mysql-configurator/grants.conf", bcolors.OKBLUE)
                defaults = {}
                try:
                    f = file('/etc/mysql-configurator/grants.conf', 'r')
                except IOError as fail:
                    self.tools.msg("/etc/mysql-configurator/grants.conf exists but couldn't read.", bcolors.FAILRED)
                for line in f:
                    k, v = line.split('=')
                    k = k.strip()
                    v = v.strip()
                    defaults.update({k: v})
                f.close()

                if defaults.__len__() > 1:
                    if 'remote' in defaults:
                        self.repo_remote_path = defaults.get('remote')
                        self.msg("Use configured remote repository path: " + self.repo_remote_path, bcolors.OKBLUE)
                    if 'local' in defaults:
                        self.repo_local_path = defaults.get('local')
                        self.msg("Use configured local repository path: " + self.repo_local_path, bcolors.OKBLUE)
            else:
                prj = self.get_project()
                if prj == None:
                    self.msg("Failed to get conductor project for this host. It is needed to define git repository for grants. Exit...", bcolors.FAILRED)
                    return None
                else:
                    # use media defaults
                    self.msg("Using default repository parameters. To override it use /etc/mysql-configurator/grants.conf.", bcolors.FAILRED)
                    self.repo_remote_path = "git@github.yandex-team.ru:salt-media/" + prj + "-secure.git"
                    self.repo_local_path = "/tmp/mysql_config_tmp"

            name = self.tools.get_grants_filename()
            if name != None:
                self.path = self.repo_local_path + "/mysql/" + name
            else:
                return None

    def get_conf(self, path):

        if self.legacy:
            cmd = "/usr/bin/secget " + path
            try:
                run = subprocess.check_output(cmd, shell=True)
                if run.__len__() > 0:
                    data = StringIO.StringIO(run)
                    return data
                else:
                    return None
            except subprocess.CalledProcessError as fail:
                self.msg("Failed exec " + cmd + " " + fail.__str__(), bcolors.FAILRED)
                return None
        else:
            try:
                temp = open(path)
                data = temp.read()
                temp.close()
            except IOError as fail:
                self.msg("getconf: Failed to open file " + path + " Error: " + fail.__str__(), bcolors.FAILRED)
                return None
            conf = StringIO.StringIO(data)
            return conf

    def get_existing_users(self):
        if self.conn is None:
            if os.path.exists("/root/.my.cnf"):
                try:
                    self.conn = MySQLdb.connect(host='localhost', read_default_file="/root/.my.cnf", )

                    cursor = self.conn.cursor()
                except MySQLdb.OperationalError as fail:
                    self.msg("MySQL connection using defaults file failed: " + fail.__str__(), bcolors.FAILRED)
                    return None
            else:
                try:
                    self.conn = MySQLdb.connect('localhost', 'root', '')

                    cursor = self.conn.cursor()
                except MySQLdb.OperationalError as fail:
                    self.msg("MySQL connection using no passwd failed: " + fail.__str__(), bcolors.FAILRED)
                    return None

        q = 'select User,Host from mysql.user;'
        cursor = self.conn.cursor()

        cursor.execute(q)
        data = cursor.fetchall()
        # build user from tuple to username@host notation
        cdata = []
        for u in data:
            cdata.append(u[0] + '@' + u[1])

        self.existing_users =  cdata

    def print_users(self):
        if self.users is None:
            self.fetch_config()

        for u in self.users:
            print("%s %s" % (u, self.users[u]))

    def get_project(self):

        url = "https://c.yandex-team.ru/api/hosts2projects/" + self.me
        try:
            data = requests.get(url)
            if data.status_code != 200:
                self.msg('Conductor request failed with HTTP status ' + data.status_code, bcolors.FAILRED)
                return None
            else:
                return data.content.strip()
        except requests.RequestException as fail:
            self.msg("Conductor request failed: " + fail.__str__(), bcolors.FAILRED)
            return None

    def get_includes(self, conf):
        includes = []
        tmpcnf = []

        if isinstance(conf, str):
            rawconf = self.get_conf(conf)
            if rawconf == None:
                return includes
        elif isinstance(conf, StringIO.StringIO):
            rawconf = conf
            conf.seek(0)
        else:
            # print conf
            self.msg("Unknown argument in get_includes: " + conf, bcolors.FAILRED)
            return None

        if rawconf is not None:
            includes.append(rawconf)

        for line in rawconf:
            if re.match("^@include+\ [a-zA-Z0-9./]+", line):
                newinc = os.path.dirname(conf) + '/' + line.split(" ")[1].replace("\n", "")
                #tmp = self.get_includes(newinc)
                #tmpcnf = self.get_conf(newinc)
                tmp = self.get_includes(newinc)
                if tmp == None:
                    return None
                else:
                    includes.extend(tmp)

        # includes.append(rawconf)

                #if tmpcnf != None:
                #     tmpcnf = self.get_includes(newinc)

        if includes.__len__() > 0:
            return includes
        else:
            return None

    def fetch_config(self):

        self.get_path()

        if not self.legacy:
            if os.path.exists(self.repo_local_path):
                # clean local copy before actions
                self.clean()
            # do git clone if new way changed (for old way no need to fetch any files)
            try:
                repo = git.Repo.clone_from(self.repo_remote_path, self.repo_local_path)
            except:
                self.msg("Failed to clone remote repository %s to local path %s. Can't continue processing without config. Exit..." % (self.repo_remote_path, self.repo_local_path), bcolors.FAILRED)
                return None

        # print generated path to grants for debug
        self.msg(self.path, bcolors.DEFAULT)

        _configs = self.get_includes(self.path)

        for config in _configs:
            h = self.read_hosts(config)
            # print h
            u = self.read_users(config)
            # fill hosts
            if h != None:
                for host in h:
                    if not host in self.hosts:
                        self.hosts.update({host: h[host]})
                    else:
                        # may be len(h[host])?
                        if h[host] != None:
                            for addr in h[host]:
                                if not addr in self.hosts[host]:
                                    self.hosts[host].append(addr)

            # fill users
            if u != None:
                for user in u:
                    # override user if it defined not once
                    self.users.update({user: u[user]})

        # fill grants
        for config in _configs:
            g = self.read_grants(config)

            if g != None:
                for grant in g:
                    if not grant in self.grants:
                        if g[grant] != None:
                            self.grants.update({grant: g[grant]})
                    else:
                        if g[grant] != None:
                            for host in g[grant]:
                                if host not in self.grants[grant]:
                                    self.grants[grant].update({host: g[grant][host]})

        # save grants in cache if grants are not empty
        # prevent rewrite cache with empty data
        if len(self.grants) > 1:
            self.savecache()

        # remove temporary cloned git directory
        #if not self.legacy:
        #   rmdir(self.repo_local_path)
        self.clean()


    def find_section(self, _conf, sec_name):
        _conf.seek(0)
        data = _conf.readlines()
        begin = 0
        end = 0

        for line in data:
            if re.match("\[" + sec_name + "\]", line):
                begin = data.index(line) + 1
                break

        _conf.seek(begin)

        idx = begin

        while not re.match("^\[", data[idx]) and idx < len(data):
            end = idx
            idx += 1
            if idx == len(data):
                return begin, end

        return begin, end

    def is_valid_cidr(self, address):
        """Check if address is valid

        The provided address can be a IPv6 or a IPv4
        CIDR address.
        """
        try:
            # Validate the correct CIDR Address
            netaddr.IPNetwork(address)
        except netaddr.core.AddrFormatError:
            return False
        except UnboundLocalError:
            # NOTE(MotoKen): work around bug in netaddr 0.7.5 (see detail in
            # https://github.com/drkjam/netaddr/issues/2)
            return False

        # Prior validation partially verify /xx part
        # Verify it here
        ip_segment = address.split('/')

        if (len(ip_segment) <= 1 or ip_segment[1] == ''):
            return False

        return True

    def msg(self, message, color):
        # print messages
        if self.verbose:
            print(color + message + bcolors.DEFAULT)

        return True

    def mypassword(self, p):
        if p != None:
            if len(p) > 0:
                return '*' + sha1(sha1(p).digest()).hexdigest().upper()
            else:
                return None
        else:
            return None

    def net_to_mhost(self, net):
        # we should know what we get: ipv6 or ipv4
        if re.match("([\d]+\.[\d]+\.[\d]+\.[\d]+/[\d]+)", net):
            maddr = netaddr.IPNetwork(net).network.__str__() + "/" + netaddr.IPNetwork(net).netmask.__str__()
        # elif re.match("((?:[0-9a-fA-F]*:)+:/[\d]+)", net):
        elif self.is_valid_cidr(net):
            if not re.match("([\d]+\.[\d]+\.[\d]+\.[\d]+)", net):
                # print net
                tmp = net.split(":")
                if re.match("/[0-9]+", tmp[-1]):
                    # print tmp
                    tmp[-2] = "%"
                    tmp.pop(-1)
                else:
                    # print tmp
                    tmp[-1] = "%"

                maddr = ":".join(tmp)
        # find one ipv6 addr
        elif re.match("((?:[0-9a-fA-F]*:)+)", net):
            maddr = net
        # find one ipv4 addr
        elif re.match("([\d]+\.[\d]+\.[\d]+\.[\d]+)", net):
            maddr = net
        else:
            return None

        return maddr

    # input is mysql grant line like in show grants for...
    def parse_grant(self, line):

        privs = []

        m = re.search(r'(\ ON\ )', line)
        end =  m.start(1)

        privs = line[6:end].replace(', ', ',').split(',')

        # detect grant option
        if line.split()[-1] == 'OPTION':
            privs.append('GRANT OPTION')

        return privs

    def parse_passwd(self, line):
        pwd_re = re.compile('\*[A-F0-9]+')

        passwd = pwd_re.findall(line)

        if len(passwd) > 0:
            return passwd[0]
        else:
            return None

    def parse_dst(self, line):
        dst_re = re.compile('ON (`?[a-zA-Z0-9(\*|@)_\-]+`?\.`?[a-zA-Z0-9\*_\-]+`?) TO')
        dst = dst_re.findall(line)

        if len(dst) < 1:
            # try to find grants like grant proxy on ''@''
            dst_re = re.compile('ON (\'[\S]*\'@\'[\S]*\') TO')
        # try again
        dst = dst_re.findall(line)

        if len(dst) > 0:
            return dst[0]
        else:


            return None

    def read_existing_grants(self):
        if self.conn is None:
            if os.path.exists("/root/.my.cnf"):
                try:
                    self.conn = MySQLdb.connect(host='localhost', read_default_file="/root/.my.cnf", )

                    cursor = self.conn.cursor()
                except MySQLdb.OperationalError as fail:
                    self.msg("MySQL connection using defaults file failed: " + fail.__str__(), bcolors.FAILRED)
                    return None
            else:
                try:
                    self.conn = MySQLdb.connect('localhost', 'root', '')

                    cursor = self.conn.cursor()
                except MySQLdb.OperationalError as fail:
                    self.msg("MySQL connection using no passwd failed: " + fail.__str__(), bcolors.FAILRED)
                    return None

        # q = 'select User,Host,Select_priv,Insert_priv,Update_priv,Delete_priv,Create_priv,Drop_priv,Process_priv,Grant_priv,Alter_priv,Execute_priv,Repl_slave_priv,Repl_client_priv from mysql.user;'
        q = 'select User, Host from mysql.user;'
        cursor = self.conn.cursor()

        cursor.execute(q)
        data = cursor.fetchall()

        for user in data:
            uname = '\'' + user[0] + '\'' + '@' + '\'' + user[1] + '\''
            q = 'show grants for ' + uname + ';'
            try:
                cursor.execute(q)
            except:
                self.tools.msg("Can\'t get grants for user " + uname + " User will be deleted.", bcolors.FAILRED)
                # add users with no grants too
                self.ex_grants.update({uname: {'grants': ['USAGE'], 'pwd': ''}})
                pass

            # get grants for user
            user_grants = cursor.fetchall()

            passwd = ''
            grants = {}

            if user_grants != None:

                # for every grant for uname parse grants
                for rawline in user_grants:

                    line = rawline[0]

                    # try only if not foun yet
                    if passwd == '':
                        passwd = self.parse_passwd(line)

                    # None passwd == ''
                    if passwd is None:
                        passwd = ''

                    dst = self.parse_dst(line)
                    one_dst_grant = self.parse_grant(line)

                    if dst in grants:
                        for grant in one_dst_grant:
                            if not grant in grants[dst]:
                                grants[dst].append(grant)
                    else:
                        grants.update({ dst : one_dst_grant })

                    if uname in self.ex_grants:
                        if dst in self.ex_grants[uname]['grants']:
                            ex = self.ex_grants[uname]['grants'][dst]
                            new = grants[dst]
                            for g in new:
                                if g not in self.ex_grants[uname]['grants'][dst]:
                                    #self.ex_grants[uname]['grants'][dst] = list(set(ex + new))
                                    self.ex_grants[uname]['grants'][dst].append(g)
                            #self.ex_grants[uname][dst] = list(set(e + n))
                        else:
                            self.ex_grants[uname]['grants'].update({dst: grants[dst]})
                    else:
                        self.ex_grants.update( {uname: {'grants': grants, 'pwd': passwd } } )
            else:
                # add users with no grants too
                self.ex_grants.update({uname: {'grants': ['USAGE'], 'pwd': '' } })


    def read_hosts(self, conf, raw=False):
        begin, end = self.find_section(conf, "hosts")

        if begin == end:
            return None

        conf.seek(0)
        data = conf.readlines()

        if not raw:
            opt = re.compile("([^\t\n][\S]+|[\*\%])")
            hosts = {}

            for i in range(begin, end):
                option = opt.findall(data[i])

                if len(option) > 1:
                    if option[0] in hosts.keys():
                        hosts[option[0]].append(option[1].strip(" "))
                    else:
                        hosts.update({option[0]: [option[1].strip(" ")]})

        else:
            hosts = StringIO.StringIO()
            for i in range(begin, end):
                hosts.write(data[i])

        return hosts

    def read_users(self, conf, raw=False):

        begin, end = self.find_section(conf, "users")

        if begin == end:
            return None

        conf.seek(0)
        data = conf.readlines()

        if not raw:
            opt = re.compile("^[a-zA-Z0-9_\-?]+(?:[\s]+)[\S]*")

            users = {}

            for i in range(begin, end):
                option = opt.findall(data[i])
                if len(option) > 0:
                    option = option[0].split()

                if len(option) > 0:
                    # check user with empty password
                    if len(option) == 1:
                        option.append(" ")
                    if option[0] in users.keys():
                        users[option[0]] = option[1].strip(" ")
                    else:
                        users.update({option[0]: option[1].strip(" ")})
        else:
            users = StringIO.StringIO()
            for i in range(begin, end):
                users.write(data[i])

        return users

    def read_cached_config(self):
        u = StringIO.StringIO()
        with open(self.cache_path + '_users', 'r') as cfg:
            self.users = yaml.load(cfg.read())
        cfg.close()
        with open(self.cache_path + '_hosts', 'r') as cfg:
            self.hosts = yaml.load(cfg.read())
        cfg.close()
        with open(self.cache_path + '_grants', 'r') as cfg:
            self.grants = yaml.load(cfg.read())
        cfg.close()


    # returns dict with grants: grants { user: { { host1: [addrlist], host2: [addrlist2]... }, grants : { db.table : [S,U] } } }
    # gets conf as sringio, and users and hosts as dict
    def read_grants(self, conf):

        conf.seek(0)
        begin, end = self.find_section(conf, "grants")

        if begin == end:
            return None

        conf.seek(0)

        data = conf.readlines()
        opt = re.compile("^[a-zA-Z0-9@_\-]+[\s]+[a-zA-Z-0-9_\-\*\.]+[\s]+[a-zA-Z\*\,_]+")

        grants = {}

        for i in range(begin, end+1):
            option = opt.findall(data[i])
            if len(option) > 0:
                option = option[0].split()
                try:
                    user, host = option[0].split("@")
                except:
                    self.msg("Failed to parse option: " + option.__str__(), bcolors.FAILRED)

                # all processing have a reason only if user defined in grant string is defined in users and host defined in hosts
                if user in self.users:

                    if host in self.hosts:
                        dst = option[1]
                        priv = option[2].split(",")
                        try:
                            grant = {dst: priv}
                        except:
                            self.msg("Failed to parse grant in line %s" % option.__str__(), bcolors.FAILRED)

                        if not user in grants:
                            if host in self.hosts:
                                if self.hosts[host] != None:
                                    try:
                                        grants.update({user: {host: {'ip': self.hosts[host], 'grants': grant}}})
                                    except:
                                        self.msg("Failed to read grant for user %s, host %s, grant: %s" % (user, host, grant.__str__()), bcolors.FAILRED)
                        else:
                            if not host in grants[user]:
                                try:
                                    grants[user].update({host: {'ip': self.hosts[host], 'grants': grant}})
                                except:
                                    self.msg("Failed to read grant for user %s, host %s, grant: %s %s" % (user, host, dst, grant), bcolors.FAILRED)
                            else:
                                if not dst in grants[user][host]['grants']:
                                    try:
                                        grants[user][host]['grants'].update(grant)
                                    except:
                                        self.msg("Failed to read grant for user: " + user + " host: " + host + " grant: " + grant.__str__(), bcolors.FAILRED)
                    else:
                        self.msg('Skip processing grants for undefined host: ' + host + '. It\'s grants will be revoked and all existing user@host will be dropped!', bcolors.WARNYELLOW)
                else:
                    self.msg('Skip processing grants for undefined user: ' + user + '. It\'s grants will be revoked and all existing user@host will be dropped!', bcolors.WARNYELLOW)

        return grants

    def build_grans_sql(self):

        existing = self.ex_grants
        new = self.b_users


        ReverseDef = {
            'S': 'SELECT',
            'D': 'DELETE',
            'U': 'UPDATE',
            'I': 'INSERT',
            'PROCESS': 'PROCESS',
            'EXECUTE': 'EXECUTE',
            'GRANT': 'GRANT OPTION',
            'REPLICATION_SLAVE': 'REPLICATION SLAVE',
            'REPLICATION_CLIENT': 'REPLICATION CLIENT',
            'CREATE_TEMPORARY_TABLES': 'CREATE TEMPORARY TABLES',
            '*': 'ALL PRIVILEGES',
            'RELOAD': 'RELOAD',
            'LOCK_TABLES': 'LOCK TABLES',
            'ALTER': 'ALTER',
            'DROP': 'DROP',
            'CREATE': 'CREATE',
            'PROXY': 'PROXY',
            'USAGE' : 'USAGE',
            'INDEX' : 'INDEX'
        }


        PrivsDef = {
            'SELECT': 'S',
            'DELETE': 'D',
            'UPDATE': 'U',
            'INSERT': 'I',
            'PROCESS': 'PROCESS',
            'EXECUTE': 'EXECUTE',
            'GRANT OPTION': 'GRANT',
            'RELOAD': 'RELOAD',
            'LOCK TABLES': 'LOCK_TABLES',
            'REPLICATION SLAVE': 'REPLICATION_SLAVE',
            'REPLICATION CLIENT': 'REPLICATION_CLIENT',
            'CREATE TEMPORARY TABLES': 'CREATE_TEMPORARY_TABLES',
            'ALL PRIVILEGES': '*',
            'ALTER': 'ALTER',
            'DROP': 'DROP',
            'CREATE': 'CREATE',
            'CREATE TABLES': 'CREATE_TABLES',
            'PROXY' : 'PROXY',
            'USAGE': 'USAGE',
            'INDEX': 'INDEX'
        }


        SkipList = [
#            'GRANT OPTION',
            'USAGE'
        ]

        # process existing grants, compare with new
        for user in existing:
            if user not in new:
                my_command = 'DROP USER ' + user + ';'
                self.sql.append(my_command)
            else:
                # check for different passwords
                if existing[user]['pwd'] != new[user]['pwd']:
                    my_command = 'SET PASSWORD for ' + user + ' = \'' + new[user]['pwd'] + '\';'
                    self.sql.append(my_command)

                for dst in existing[user]['grants']:
                    if dst not in new[user]['grants']:
                        rgrants = ""
                        for grant in existing[user]['grants'][dst]:
                            if grant not in SkipList:
                                #rgrants = rgrants + PrivsDef[grant] + ','
                                rgrants = rgrants + grant + ','

                        rgrants = rgrants[0:-1]
                        if rgrants != '':
                            # any existing user have USAGE grant on *.*, this must be skipped
                            if rgrants != "USAGE" and dst != "*.*":
                                my_command = 'REVOKE ' + rgrants + ' ON ' + dst + ' FROM ' + user + ';'
                                self.sql.append(my_command)
                    else:
                        rgrants = ""
                        for grant in existing[user]['grants'][dst]:

                            if PrivsDef[grant] not in SkipList:
                                if PrivsDef[grant] not in new[user]['grants'][dst]:
                                    # rgrants = rgrants[0:-1] + grant + ','
                                    rgrants = rgrants + grant + ','

                        if rgrants != '':
                            # remove ',' in the end
                            rgrants = rgrants[:-1]

                            # any existing user have USAGE grant on *.*, this must be skipped
                            #if (rgrants != "USAGE" and dst != "*.*"):
                            my_command = 'REVOKE ' + rgrants + ' ON ' + dst + ' FROM ' + user + ';'
                            self.sql.append(my_command)

        # process new grants, compare with existing
        for user in new:
            # in first we should create user if it doesn't exists
            if not user in existing:
                my_command = 'CREATE USER ' + user + ' IDENTIFIED BY PASSWORD ' + '\'' + new[user]['pwd'] + '\'' + ';'
                self.sql.append(my_command)
                # create all for new user
                for dst in new[user]['grants']:
                    grants = ""
                    if 'GRANT' in new[user]['grants'][dst]:
                        grant_opt = 'WITH GRANT OPTION'
                    else:
                        grant_opt = ''

                    for grant in new[user]['grants'][dst]:
                        if grant != 'GRANT':
                            # if grant option found in new grants and not in existing
                            grants = grants + ReverseDef[grant] + ','

                    if grants != '':
                        my_command = 'GRANT ' + grants[0:-1] + ' ON ' + dst + ' TO ' + user + ' ' + grant_opt + ';'
                        self.sql.append(my_command)

            else:
                # now look for destinations
                for dst in new[user]['grants']:
                    if dst not in existing[user]['grants']:
                        # if destination not exists for this user, add it and grants for it
                        grants = ""
                        if 'GRANT' in new[user]['grants'][dst]:
                            grant_opt = 'WITH GRANT OPTION'
                        else:
                            grant_opt = ''
                        for grant in new[user]['grants'][dst]:
                            if grant != 'GRANT':
                                # if grant option found in new grants and not in existing
                                grants = grants + ReverseDef[grant] + ','

                        if grants != '':
                            my_command = 'GRANT ' + grants[0:-1] + ' ON ' + dst + ' TO ' + user + ' ' + grant_opt + ';'
                            self.sql.append(my_command)
                    else:
                        # dst exists in existing grants for this user, we should add grants for it if needed
                        grants = ""
                        if 'GRANT' in new[user]['grants'][dst] and 'GRANT' not in existing[user]['grants'][dst]:
                            grant_opt = 'WITH GRANT OPTION'
                        else:
                            grant_opt = ''

                        for grant in new[user]['grants'][dst]:
                            # here may be just grant instead of PrivsDef[grant]?????
                            if ReverseDef[grant] not in existing[user]['grants'][dst]:
                                # grant option requires special processing
                                if grant != 'GRANT':
                                    # if grant option found in new grants and not in existing
                                    grants = grants + ReverseDef[grant] + ','

                        if grants != '':
                            my_command = 'GRANT ' + grants[0:-1] + ' ON ' + dst + ' TO ' + user + ' ' + grant_opt + ';'
                            self.sql.append(my_command)

    def update_grants(self):

        if not self.dryrun:
            if self.conn is None:
                if os.path.exists("/root/.my.cnf"):
                    try:
                        self.conn = MySQLdb.connect(host='localhost', read_default_file="/root/.my.cnf",)

                        cursor = self.conn.cursor()
                    except MySQLdb.OperationalError as fail:
                        self.msg("MySQL connection using defaults file failed: " + fail.__str__(), bcolors.FAILRED)
                        return None
                else:
                    try:
                        self.conn = MySQLdb.connect('localhost', 'root', '')

                        cursor = self.conn.cursor()
                    except MySQLdb.OperationalError as fail:
                        self.msg("MySQL connection using no passwd failed: " + fail.__str__(), bcolors.FAILRED)
                        return None

            else:
                cursor = self.conn.cursor()

            # disable binary logging to prevent replication of grants statements
            self.sql.insert(0, 'set sql_log_bin=0;')
            # enable back binary logging
            self.sql.append('set sql_log_bin=1;')

            # workaround for mysql bug on CREATE USER: error 1396 hy000 operation create user failed
            # http://www.mindflow.su/linux/fixing-error-1396-hy000-operation-create-user-failed/
            # disabled: brokes grants
            # commands = ['DELETE FROM mysql.db;', 'FLUSH PRIVILEGES;']
            commands = ['FLUSH PRIVILEGES;']
            for c in commands:
                try:
                    cursor.execute(c)
                    self.msg('EXECUTE: ' + c, bcolors.OKGREEN)
                except:
                    self.msg('FAILED: ' + c, bcolors.FAILRED)
                    pass

        for command in self.sql:
            if self.dryrun:
                self.msg('DRYRUN: ' + command, bcolors.PURRPLE)
            else:
                try:
                    cursor.execute(command)
                    self.msg('EXECUTE: ' + command, bcolors.OKGREEN)
                except:
                    self.tools.msg('Command execution failed: ' + command, bcolors.FAILRED)
                    self.failed += 1
                    pass

    def clean(self):
        if self.repo_local_path is not None:
            if os.path.exists(self.repo_local_path):
                protect = ['/', '/usr', '/var', '/lib' ]
                if self.repo_local_path in protect:
                    self.msg('ACHTUNG! Dangerous operation: recursive delete ' + self.repo_local_path + ' won\'t be executed!', bcolors.FAILRED)
                    return 1
                for root, dirs, files in os.walk(self.repo_local_path, topdown=False):
                    for name in files:
                        os.remove(os.path.join(root, name))
                    for name in dirs:
                        os.rmdir(os.path.join(root, name))

    def savecache(self):
        if not os.path.exists(self.cache_path):
            os.mkdir(self.cache_path, 0700)

        tmp = StringIO.StringIO(self.grants)
        self.tools.savedata(self.cache_path + '/_grants', tmp)
        tmp = StringIO.StringIO(self.hosts)
        self.tools.savedata(self.cache_path + '/_hosts', tmp)
        tmp = StringIO.StringIO(self.users)
        self.tools.savedata(self.cache_path + '/_users', tmp)
