"""
Working with grants: read, update, save config
Uses git and secdist as configs storages
"""

import re
import os
import stat
from copy import copy
import socket
import logging
import yaml
import requests
from mysql_configurator.mysql import MySQL, MySQLError
from mysql_configurator.grants.utils import resolve_host, is_valid_cidr, listreduce, secget
from mysql_configurator.grants.utils import git_clone, clean, net_to_mhost, mypassword, merge
from mysql_configurator.grants.utils import formatuser
from mysql_configurator.grants.utils import yav_get


class MysqlWorker(MySQL):
    """
    Class for working with mysql and current existing grants in it
    """
    def __init__(self, host='localhost'):  # pylint: disable=super-init-not-called
        self.log = logging.getLogger(self.__class__.__name__)
        self.host = host
        self.conn = None

    def get_users(self):
        """
        load current users from mysql
        :return: dict with users and password hashes
        """
        query = "select CONCAT('{',QUOTE(CONCAT(user,'@',host))," \
                "':',QUOTE(authentication_string),'}')"\
                " from mysql.user;"
        if not self.is_connected():
            try:
                self.connect(host=self.host)
            except MySQLError as err:
                self.log.debug(err)
                raise MySQLError

        data = self.query(query, as_dict=False, raw=True)
        # convert result into dict
        users = {}
        for result in data:
            users.update(yaml.load(result[0]))
        return users

    @staticmethod
    def get_privelege(line):
        """
        parsing mysql grant notation in show grants for
        :param line:
        :return: list of privileges
        """
        if 'PROCEDURE' in line.split():
            end = re.search(r'(\ ON\ PROCEDURE)', line).end(1)
        else:
            end = re.search(r'(\ ON\ )', line).start(1)
        privs = line[6:end].replace(', ', ',').split(',')

        # detect grant option
        if line.split()[-1] == 'OPTION':
            privs.append('GRANT OPTION')

        return privs

    def parse_dst(self, line):
        """
        parsing privelege destination in show grants line
        :param line:
        :return: destination
        """
        try:
            dst = re.search(r'ON\s+(?:PROCEDURE\s+)?([\\`\'\w\*\-\.\@]+)\s+TO', line).group(1)
        except AttributeError:
            self.log.debug("Can't find destination in line %s", line)
            return None
        else:
            return dst.replace('`', '')

    def get_user_privs(self, user):
        """
        get privileges for user from mysql
        :param user: username in mysql format
        :return: dict with privileges
        """
        _uname = user.split('@')
        query = "show grants for %s" % "`{}`@`{}`".format(_uname[0], _uname[1])
        grants = {}
        if not self.is_connected():
            self.connect(host=self.host)

        try:
            grantlines = self.query(query, as_dict=False, raw=True)
        except MySQLError as err:
            self.log.debug(err)
            return {}

        for line in grantlines:
            # regexp in condition is a required magic to skip the grants inside more global grant
            # example: skip line with 'username'@'%' for show grants for 'username'@'127.0.0.1';
            if re.findall(r".{}.@.{}.".format(_uname[0], _uname[1]), line[0]):
                priv = self.get_privelege(line[0])
                dst = self.parse_dst(line[0])
                grants.update({dst: priv})
        return grants

    def get_all_privs(self):
        """
        Get all users privileges
        :return: dict with privileges like this
        {'username@%': {'database.*': ['SELECT', 'CREATE TEMPORARY TABLES', 'EXECUTE']}}
        """
        self.log.debug("Get existing grants from mysql.")
        privs = {}
        users = self.get_users()
        if not users:
            self.log.debug("Can't load MySQL users from server.")
            return None

        for user in users:
            privs[user] = self.get_user_privs(user)

        self.log.debug("Got existing grants: %s", privs)
        return privs

    def execute(self, query, as_dict=False, raw=False):
        """
        simple wrapper for execute MySQL query
        :param query: query to exec
        :param as_dict: return output as dict (inherit from MySQL)
        :param raw: as raw data (inherit from MySQL)
        :return: query result
        """
        if not self.is_connected():
            try:
                is_success = self.connect(host=self.host)
                # need for try to reconnect
                if not is_success:
                    self.conn = None
                    return None
            except MySQLError as err:
                self.log.debug(err)
                return None

        try:
            data = self.query(query, as_dict=as_dict, raw=raw)
        except MySQLError as error:
            self.log.debug(error)
            # need for try to reconnect
            self.conn = None
            return None

        return data

    @property
    def get_version(self):
        """
        gets mysql server version
        :return: mysql version
        """
        data = self.execute('SELECT @@version;')
        try:
            return data[0]['@@version']
        except (KeyError, TypeError):
            return '5.7'


class ConfigProcessor(object):
    """
    Grants config processing class
    """
    def __init__(self, config):
        self.log = logging.getLogger(self.__class__.__name__)
        self.config = config
        self.api_host = self.config.grants.apihost or 'c.yandex-team.ru'
        self.c_project_root = self.get_c_project_root()
        self.c_config_path = self.get_c_config_path()
        self.path = self.config.grants.path or self.get_config_path()
        if self.config.grants.cache_root:
            self.cache_path = os.path.join(self.config.grants.cache_root, "/grants_cache.yml")
        else:
            self.cache_path = "/tmp/grants_cache.yml"
        self.grantconfig = None
        self.ignore_errors = self.config.grants.ignore_errors

    def unset_ignore_errors(self):
        """
        set ignore_errors to False (default: config value)
        :return:
        """
        self.ignore_errors = False

    def set_ignore_errors(self):
        """
        set ignore_errors to True (default: config value)
        :return:
        """
        self.ignore_errors = True

    def request(self, url):
        """
        simple wrapper for requests usage. returns content
        :param url: url ti get
        :return: data.content
        """
        try:
            data = requests.get(url, verify='/etc/ssl/certs/ca-certificates.crt')
            if data.status_code != 200:
                msg = "Request {} failed with response status {}.".format(url, data.status_code)
                self.log.debug(msg)
                if not self.ignore_errors:
                    raise Exception(msg)
                return None
            else:
                return data.content
        except (requests.ConnectionError, requests.HTTPError) as fail:
            self.log.debug(fail.__str__())
            if not self.ignore_errors:
                raise Exception(fail.__str__())

    def group2ip(self, group):
        """
        resolve conductor group into IP addresses
        :param group: conductor group to resolve
        :return: IP addresses
        """
        addrs = None
        hosts = self.group2hosts(group)
        if hosts:
            addrs = list(map(resolve_host, hosts))
        return addrs

    def group2hosts(self, group):
        """
        resolves conductor group into the list of hosts
        :param group: c. group
        :return: list of hosts
        """
        group = group.replace('%', '')
        url = "https://c.yandex-team.ru/api/groups2hosts/" + group
        data = self.request(url)

        if not data:
            return None

        return data.split()

    def expand_macros(self, macro):
        """
        expands FW macro into ipaddrs list
        :param macro: FW macro
        :return: list of addresses
        """
        if not macro:
            return None

        def projectid_net_map(item):
            """project_id support(paulus@)"""
            matched = re.match(r'(\w+)@(2a02:6b8:c..)([\w:]*)/40', item)
            # old format return before changes for CADMIN-7829
            # return '2a02:6b8:c__:%' + matched.group(1) + '%' if matched else item
            return "2a02:6b8:c__:%{}%".format(matched.group(1)) if matched else item

        # url = "http://ro.admin.yandex-team.ru/api/firewall/expand_macro.sbml?macro=" + macro
        url = "http://hbf.yandex.net/macros/" + macro
        ips = None

        # detect raw mysql host definition like 192.168.% or 2a02:6b8:c__:%423f%
        if re.match(r"^[a-fA-F0-9\.\:%_]+$", macro):
            ips = macro
        # process localhost: it can be defined as localhost or addr
        # elif macro == "localhost":
        #     return [macro, resolve_host(macro)]
        # detect not resolvable definithion like procedure_host and etc
        elif re.match(r'^[a-z_\-0-9]+$', macro):
            ips = macro
        # got fqdn
        elif re.match(r'^[a-z0-9\.\-]+$', macro):
            ips = resolve_host(macro)
        # got conductor group
        elif re.match(r'^%[a-z0-9_\-]+$', macro):
            ips = self.group2ip(macro)
        # network macroces
        elif re.match(r'^[A-Z0-9_]+$', macro):
            data = yaml.load(self.request(url))
            # following magic needed for resolving conductor macroses with valid host addrs
            # like _C_MYGROUP_
            # and firewall macroces with network addrs
            # like _SHARESANDBOX_
            if data:
                ips = []
                # find ip networks
                for val in data:
                    # if not re.match(r"[a-zA-Z0-9\.?\-?]{5,}\.?[a-z]+", val):
                    if is_valid_cidr(val):
                        ips.append(val)
                    else:
                        ips.append(projectid_net_map(val))
        else:
            self.log.warn("Unknown macro: %s", macro)
            return None

        return ips

    def get_fqdn(self):
        """
        Get FQDN
        :return: FQDN
        """
        try:
            return socket.gethostname()
        except socket.error as fail:
            msg = "Can't get hostname. " + fail.__str__()
            self.log.error(msg)
            raise Exception(msg)

    def get_c_project_root(self):
        """
        Get project root dir for generate grants config path on secdist using conductor API
        :return: string with path
        """
        project_root = self.request("http://c.yandex-team.ru/api/generator/mysql_secure_repository?fqdn={}".format(self.get_fqdn())).replace("\"", "")
        self.log.debug("Got project root from Conductor API: \"%s\"", project_root)
        return project_root if project_root else None

    def get_c_config_path(self):
        """
        Get config path for generate grants config path on secdist using conductor API
        :return: string with path
        """
        config_path = self.request("https://{}/api/generator/mysql_grants_file?fqdn={}".format(self.api_host, self.get_fqdn())).replace("\"", "")
        self.log.debug("Got config path from Conductor API: \"%s\"", config_path)
        return config_path if config_path else None

    def get_config_path(self):
        """
        Legacy method to generate grants config path on secdist using conductor API
        :return: api response string(content)
        """
        if not self.c_project_root:
            self.log.info("Failed to get project root path on secdist.")
            return None

        if not self.c_config_path:
            self.log.info("Failed to get grants config path on secdist.")
            return None

        return "{}/{}".format(self.c_project_root, self.c_config_path)

    def loadconfig(self, resource, use_cache=True):
        """
        loads yaml config from screcified resource:
        if local - use local path to load config
        if vault secret - use yandex vault
        :param resource: secdist or local filenane, or yav secret
        :param use_cache: use cache or not
        :return: config as dict
        """
        def yaml_include_loader(loader, node):
            """loader for include configs"""
            self.log.debug("Load node %s, loader: %s", node, loader)
            path = os.path.join(os.path.dirname(resource), node.value)
            self.log.debug("include config %s", path)
            data = yaml.load(open(path))
            return data

        def yaml_include_loader_yav(loader, node):
            """loader for include configs for configs from yav"""
            self.log.debug("Load node %s, loader: %s", node, loader)
            secret, key = node.value.split(":", 2)
            return yaml.load(yav_get(secret)[key])

        def postprocess(config):
            """convert user definitions of whole db to mysql notation"""
            self.log.debug("Run grants postprocessing(fixes user db definitions like 'db').")
            for item in config['grants']:
                if re.match(r'[\w]+$', item['db']):
                    dst = item['db']
                    self.log.info("Process: %s -> %s.*.", dst, dst)
                    item['db'] += '.*'
            return config

        def load_from_cache():
            if os.path.exists(self.cache_path):
                self.log.debug('Use cached config %s', self.cache_path)
                try:
                    cachedconfig = open(self.cache_path, 'r')
                    result = yaml.safe_load(cachedconfig.read())
                    if result:
                        return result
                    else:
                        msg = "Wrong configuration in cache {}.".format(self.cache_path)
                        self.log.error(msg)
                        raise Exception(msg)
                except (OSError, yaml.YAMLError) as err:
                    self.log.error("Failed to load cached config:: %s", err)
                    raise Exception(err)

        def load_from_file(filename):
            yaml.add_constructor('!include', yaml_include_loader)

            if os.path.exists(filename):
                self.log.debug("Load local config %s", filename)
                result = yaml.load(open(filename, 'r').read())
            else:
                self.log.info("Local grants config %s not found. Try secdist path.", filename)
                self.log.debug("Load secdist config %s", filename)
                result = yaml.load(secget(filename))

            if not self.check(result):
                self.log.error("Can't process with incorrect config.")
                self.log.debug("Config file: %s; config: %s", filename, str(result))
                raise Exception("Incorrect config.")
            return postprocess(result)

        def load_from_yav(secret):
            yaml.add_constructor('!include', yaml_include_loader_yav)

            self.log.info("Load grants config from Yav secret: %s", secret)

            if ":" in secret:
                secret_uuid, key = secret.split(":", 2)
                self.log.debug("Load config from Yav secret: %s[%s]", secret, key)
                result = yav_get(secret_uuid)[key]
            else:
                secret_uuid = secret
                yav_secret = yav_get(secret_uuid)

                self.log.info("Key for yav secret is't setted, try use fallback keys")
                if self.c_config_path and self.c_config_path in yav_secret:
                    key = self.c_config_path.replace("/", ".")
                    self.log.info("Got data from key from config path from Conductor: \"%s\"", key)
                    self.log.debug("Load config from Yav secret: %s[%s]", secret_uuid, key)
                    result = yav_secret[key]
                elif "mysql_grants" in yav_secret:
                    # Fallback to default key
                    self.log.info("Got data from default key: \"mysql_grants\"")
                    self.log.debug("Load config from Yav secret: %s[%s]", secret_uuid, "mysql_grants")
                    result = yav_secret["mysql_grants"]
                else:
                    raise Exception("Don't know from which key in secret data is located")

            # Prepare for include directive for keys with current secret
            # Can be: "__this__:key" or ":key"
            result = result.replace("__this__", secret_uuid)
            result = re.sub(r"\!include\s*\:", "!include %s:" % secret_uuid, result)
            result = yaml.load(result)

            if not self.check(result):
                self.log.error("Can't process with incorrect config.")
                self.log.debug("Config secret: %s; config: %s", secret, str(result))
                raise Exception("Incorrect config.")

            return postprocess(result)

        if use_cache:
            result = load_from_cache()
            if result:
                return result

        if re.search(r"^sec-", resource):
            result = load_from_yav(resource)
        else:
            result = load_from_file(resource)

        return result


    @staticmethod
    def check(config):
        """
        Simple check config for empty sections or empty whole config
        :param config:
        :return: bool
        """
        log = logging.getLogger("CheckConfig")
        log.info("Checking config.")

        if config is None:
            log.error("Nothing instead of config O_o!")
            return False

        for key in ['grants', 'users', 'hosts']:
            if key not in config:
                log.error("Config incorrect: not found key '%s'.", key)
                return False
            if not config[key]:
                log.error("Config incorrect: empty key '%s'.", key)
                return False
        log.info("Config check passed.")
        return True

    def getconfig(self):
        """
        init grants config and returns it
        :return: grants config
        """
        path = self.path
        temp = None
        # if git url is defined use git instead of secdist
        if self.config.grants.url:
            self.log.info("Use git url %s", self.config.grants.url)
            temp = "/tmp/_mysql_grants_update-4"
            if not os.path.exists(temp):
                os.mkdir(temp)
            self.log.debug("Clone %s to %s", self.config.grants.url, temp)
            git_clone(self.config.grants.url, temp)
            path = os.path.join(temp, self.path)
            self.log.info("Set grants config path to %s", path)

        # if secret_uuid is defined use vault_client
        if self.config.grants.yav_secret:
            self.log.info("Yav secret: %s", self.config.grants.yav_secret)
            path = self.config.grants.yav_secret

        self.grantconfig = self.loadconfig(path, use_cache=bool(self.config.grants.cached))
        if temp:
            # clean temp dir
            clean(temp)
        return self.grantconfig

    def savecache(self):
        """
        Save grants configuration in cache
        :return: None
        """
        self.log.info("Save config cache to %s", self.cache_path)
        try:
            cachefile = open(self.cache_path, 'w')
            cachefile.write(yaml.safe_dump(self.grantconfig))
            cachefile.close()
            os.chmod(self.cache_path, stat.S_IRUSR | stat.S_IWUSR | stat.S_IRGRP)
            self.log.info("Config cache saved.")
        except OSError as error:
            self.log.warn("Failed to load cache data to %s: %s", self.cache_path, error.message)


class Grantushka(object):  # pylint: disable=too-many-instance-attributes
    """
    Class for working with new configured grants and updating it.
    Generates sql commnads and converts config definitions into mysql format commands.
    Grants config initializes in class constructor: on init we have the config with grants
    ToDo: replace command generators and version checker to MysqlWorker
    """
    def __init__(self, config):
        self.log = logging.getLogger(self.__class__.__name__)
        self.serverversion = None
        self.config = config
        self.confworker = ConfigProcessor(self.config)
        self.myworker = MysqlWorker(host=config.grants.override_mysql_host or 'localhost')
        self.commands_list = None
        self.hosts = None
        self.newusers = None
        self.newprivs = None
        self.oldprivs = None
        self.oldusers = None
        self.conf = self.confworker.getconfig()
        if self.conf:
            self.confworker.savecache()
        else:
            return
        self.log.debug("Used config: %s", self.config.grants.toDict())

    @staticmethod
    def expandgrants(grants):
        """
        expands grants definition like [S,U,I,D] into [SELECT,UPDATE,INSERT,DELETE]
        :param grants: grants config section
        :return: expanded grants
        """
        shortdef = {
            'S': 'SELECT',
            'U': 'UPDATE',
            'I': 'INSERT',
            'D': 'DELETE',
            '*': 'ALL PRIVILEGES',
            'GRANT': 'GRANT OPTION',
            'G': 'GRANT OPTION'
        }
        for grant in grants:
            privs = grant.pop('privs')
            # replace old style definition like REPLICATION_CLIENT
            # privs = list(map(lambda x: x.replace('_', ' '), privs))
            privs = [x.replace('_', ' ') for x in privs]
            # replace short definitions like S into SELECT
            # result = list(map(lambda x: shortdef[x] if x in shortdef else x, privs))
            result = [shortdef[x] if x in shortdef else x for x in privs]
            grant.update({'privs': result})

        return grants

    def processhosts(self):
        """
        expand network macroces, groups and names into ip networks and hosts
        converts config[hosts] into self.hosts with addresses
        :return: None
        """
        self.log.debug("Processing hosts.")
        if self.hosts:
            return
        self.hosts = {}
        for host in self.conf['hosts']:
            reduced = []
            # raw result of expand_macros can be a composite list
            raw_result = [self.confworker.expand_macros(x)
                          for x in self.conf['hosts'][host]]
            listreduce(raw_result, reduced)
            # 2 lines above is the same as one commented line down
            # listreduce(list(map(self.confworker.expand_macros, self.conf['hosts'][host])),reduced)
            # convert network addresses to mysql compatible
            self.hosts[host] = list(map(net_to_mhost, reduced))
            self.log.debug("Resolved hosts macro %s: %s", host, self.hosts[host])

    @staticmethod
    def myuser(user, host):
        """
        creates mysql username from user and host params
        :param user:
        :param host:
        :return: mysql user
        """
        return "{}@{}".format(user, host)

    def generatemyusers(self):
        """
        generate mysql users to be created into dict
        :return: users dict {user@host : password_hash}
        """
        if not self.hosts:
            self.processhosts()

        users = {}
        for grant in self.conf['grants']:
            # todo: delete comment below if new variant without lambda works fine(CADMIN-7667)
            # for user, host in map(lambda x: (grant['user'], x), self.hosts[grant['host']]):
            user = grant['user']
            for host in self.hosts[grant['host']]:
                name = self.myuser(user, host)
                users[name] = mypassword(self.conf['users'][user])
                # dirty hack to prevent revoking privileges from root@localhost:
                # add user user@localhost to users dict if detected localho grants for user
                if grant['host'] == 'localhost':
                    name = self.myuser(user, grant['host'])
                    users[name] = mypassword(self.conf['users'][user])

        return users

    @staticmethod
    def splitmyuser(username):
        """
        generates tuple with user and host
        :param username: user@host
        :return: user, host
        """
        return username.split('@')

    def generateprivileges(self):
        """
        generates privileges for users dict according to config['grants']
        :return: dict with privileges by username
        """
        self.log.debug("Generate new privileges.")
        privs = {}
        grants = self.expandgrants(self.conf['grants'])

        for grant in grants:
            user, host = grant['user'], grant['host']
            # todo: delete comment below if new variant without lambda works fine(CADMIN-7667)
            # userlist = list(map(lambda x: self.myuser(user, x), self.hosts[host]))
            userlist = list(self.myuser(user, h) for h in self.hosts[host])
            target = "*.*" if grant['db'] == '*' else grant['db']
            user_privs = {target: grant['privs']}
            for myuser in userlist:
                if myuser in privs:
                    current = privs.pop(myuser)
                    privs[myuser] = merge(current, user_privs)
                else:
                    privs[myuser] = user_privs

        self.log.debug("Generated new privileges: %s", privs)
        return privs

    def createuser(self, user):
        """
        Generates create user mysql command
        :param user:
        :return:
        """
        if not self.serverversion:
            self.serverversion = self.getserverversion()
        ver = self.serverversion[0:3].replace('.', '')

        if int(ver) > 56:
            cmd = "CREATE USER {} IDENTIFIED WITH mysql_native_password AS \'{}\'".format(
                formatuser(user), self.newusers[user] or '')
        else:
            cmd = "CREATE USER {} IDENTIFIED BY PASSWORD \'{}\';".format(
                formatuser(user), self.newusers[user] or '')
        return cmd

    def getserverversion(self):
        """
        gets mysql server version
        :return: mysql version
        """
        data = self.myworker.execute('SELECT @@version;')
        try:
            return data[0]['@@version']
        except (KeyError, TypeError):
            return '5.7'

    def grantpriv(self, user, target, privs):
        """
        Generates grant mysql command
        :param user:
        :param target:
        :param privs:
        :return: sql command
        """
        self.log.debug("Generate GRANT statement with %s %s %s", user, target, privs)
        if not re.match(r"(\w+|\*)\.(\w+|\*)", target):
            target = target + '.*'
        if 'GRANT OPTION' in privs:
            privs.remove('GRANT OPTION')
            # if only GRANT OPTION in privileges syntax is differs
            if not privs:
                sql = "GRANT GRANT OPTION on {} to {};".format(target, formatuser(user))
            else:
                priv = ",".join(privs)
                sql = "GRANT {} ON {} TO {} WITH GRANT OPTION;".format(
                    priv, target, formatuser(user))
        else:
            priv = ",".join(privs)
            if 'PROCEDURE' in priv.split():
                sql = "GRANT {} {} TO {};".format(priv, target, formatuser(user))
            else:
                sql = "GRANT {} ON {} TO {};".format(priv, target, formatuser(user))
        return sql

    def revokepriv(self, user, target, privs):
        """
        Generates revoke mysql command
        :param user: username
        :param target: database to revoke
        :param privs: privileges to revoke
        :return: sql command
        """
        self.log.debug("Generate REVOKE statement with %s %s %s", user, target, privs)
        priv = ",".join(privs)
        if not re.match(r"(\w+|\*)\.(\w+|\*)", target):
            target = target + '.*'
        if 'PROCEDURE' in priv.split():
            sql = "REVOKE {} {} from {};".format(priv, target, formatuser(user))
        else:
            sql = "REVOKE {} on {} from {};".format(priv, target, formatuser(user))
        return sql

    def modifypassword(self, user):
        """
        Generates mysql commnad to modufy user password
        :param user: usename
        :return: sql
        """
        if not self.serverversion:
            self.serverversion = self.getserverversion()
        ver = self.serverversion[0:3].replace('.', '')
        if int(ver) > 56:
            sql = "ALTER USER {} IDENTIFIED WITH mysql_native_password AS '{}';"
        else:
            sql = "SET PASSWORD for {} = '{}';"

        return sql.format(formatuser(user), self.newusers[user] or '')

    @staticmethod
    def dropuser(username):
        """
        Generates drop user command
        :param username: username to drop
        :return: sql
        """
        return "DROP USER {};".format(formatuser(username))

    def modify_user(self, username):
        """
        Generates mysql commands list to modify user and it's privileges
        :param username: user
        :return: sql commands list
        """
        sql = set()
        newprivs = self.newprivs[username]
        oldprivs = self.oldprivs[username]
        skip = {'PROXY', 'USAGE'}
        newdst = set(newprivs.keys())
        olddst = set(oldprivs.keys())
        olddst -= {"''@''"}
        adddst = newdst.difference(olddst)
        modify = newdst.intersection(olddst)

        # revoke first!
        # https://pythonworld.ru/tipy-dannyx-v-python/mnozhestva-set-i-frozenset.html
        for dst in olddst.difference(newdst):
            revoke = set(oldprivs[dst]).difference(skip)
            if revoke:
                self.log.debug("Revoke: DB %s, privileges: %s, username: %s", dst, revoke, username)
                sql.add(self.revokepriv(username, dst, revoke))

        for dst in modify:
            old = set(oldprivs[dst]).difference(skip)
            new = set(newprivs[dst]).difference(skip)
            revoke = old.difference(new)
            if revoke:
                self.log.debug("Modify(revoke): DB %s, privileges: %s, username: %s",
                               dst, revoke, username)
                sql.add(self.revokepriv(username, dst, revoke))

        # grant only after revoke!
        for dst in adddst:
            self.log.debug("Grant: DB %s, privileges: %s, username: %s",
                           dst, newprivs[dst], username)
            sql.add(self.grantpriv(username, dst, newprivs[dst]))
        for dst in modify:
            old = set(oldprivs[dst]).difference(skip)
            new = set(newprivs[dst]).difference(skip)
            add = new.difference(old)
            if add:
                self.log.debug("Modify(grant): DB %s, privileges: %s, username: %s",
                               dst, add, username)
                sql.add(self.grantpriv(username, dst, add))

        # modify password if changed
        old = self.oldusers[username]
        new = self.newusers[username]
        if old != new:
            self.log.debug("Modify(user): %s. Old: %s, new: %s", username, old, new)
            sql.add(self.modifypassword(username))

        return sql

    def generate_sql(self):
        """
        Generates full sql commands list to update grants
        :return: sql list
        """
        self.log.debug("Generate SQL for update grants.")
        # init
        self.newusers = self.generatemyusers()
        self.oldusers = self.myworker.get_users()
        self.newprivs = self.generateprivileges()
        self.oldprivs = self.myworker.get_all_privs()
        self.log.debug("Got existing users: %s", self.oldusers)
        self.log.debug("Generated new users: %s", self.newusers)

        usersadd = set(self.newprivs.keys()).difference(set(self.oldprivs.keys()))
        usersdel = set(self.oldprivs.keys()).difference(set(self.newprivs.keys()))
        usersmod = set(self.newprivs.keys()).intersection(set(self.oldprivs.keys()))

        # skip droping of reserved users(see https://st.yandex-team.ru/CADMIN-7548)
        # changed default behavior
        if self.config.grants.reserved_users:
            reserved_users = self.config.grants.reserved_users
        else:
            reserved_users = ['root@localhost', 'mysql.sys@localhost', 'mysql.session@localhost']
        self.log.info('Reserved users will not be dropped: %s', reserved_users)
        usersdel.difference_update(reserved_users)

        self.log.debug("Users: Add: %s; Del: %s, Both exists: %s", usersadd, usersdel, usersmod)

        create_sql = []
        modify_sql = []
        drop_sql = []
        for user in usersadd:
            # add user
            self.log.debug("Add user: %s", user)
            create_sql.append(self.createuser(user))
            # add privileges
            for dst in self.newprivs[user]:
                self.log.debug("Grant: DB %s, privileges: %s, username: %s",
                               dst, self.newprivs[user][dst], user)
                # copy list object with privs to prevent modify it within grantpriv()
                # this is a reason for CADMIN-7634
                privs = copy(self.newprivs[user][dst])
                modify_sql.append(self.grantpriv(user, dst, privs))
        # modify users
        for user in usersmod:
            modify = self.modify_user(user)
            if modify:
                modify_sql.extend(modify)
        # delete users
        for user in usersdel:
            self.log.debug("Drop user: %s", user)
            # drop at first
            drop_sql.append(self.dropuser(user))

        # reverse sort needed to prevent onetime grant and revoke for the same user
        # self.commands_list = sorted(sql, reverse=True)
        self.commands_list = drop_sql + create_sql + sorted(modify_sql, reverse=True)

    def update(self):
        """
        Updates mysql grants(execute sql commands list generated by generate_sql())
        :return:
        """
        dryrun = self.config.grants.dryrun or False
        self.log.warn("Dryrun mode: %s", dryrun)
        if not self.commands_list:
            self.generate_sql()
        commands = self.commands_list
        if not commands:
            self.log.info('Generated empty sql: not found grants difference.')
            return
        commands.insert(0, "FLUSH PRIVILEGES;")
        commands.insert(0, 'SET sql_log_bin=0;')
        commands.append("FLUSH PRIVILEGES;")
        commands.append("SET sql_log_bin=1;")
        if dryrun:
            for command in commands:
                self.log.warn(command)
            return

        for command in commands:
            try:
                self.log.info(command)
                self.myworker.execute(command)
            except MySQLError as err:
                self.log.error(err)
