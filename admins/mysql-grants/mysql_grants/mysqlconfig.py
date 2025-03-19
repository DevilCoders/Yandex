import requests
import re
import ConfigParser
import os
import fnmatch
import socket
import StringIO
from getpass import getuser
import MySQLdb
from utils import *
from grants import Grants
from pprint import pprint
from time import sleep


class mycnf():
    def __init__(self, legacy, verbose, dryrun):
        self.local_config =  ConfigParser.ConfigParser(allow_no_value=True)
        self.newconfig = None
        self.clientcnf = None
        self.conffiles = []
        self.client_file = "/etc/mysql/client.cnf"
        self.dotmycnf = StringIO.StringIO("!include /etc/mysql/client.cnf\n")
        self.fqdn = socket.gethostname()
        self.me = getuser()
        self.legacy = legacy
        self.path = None
        self.verbose = verbose
        self.tools = utils(self.verbose)
        self.grants = Grants(self.legacy, self.verbose, True)
        self.dryrun = dryrun
        self.mycnf = None
        self.l_mycnf = None
        self.config_diff = None
        self.timeout = 300

    def apply_config(self):

        # transform data types to bytes
        for opt in self.config_diff:
            if re.match('^[0-9]+(K|M|G)+', str(self.config_diff[opt])):
                self.config_diff.update({opt: self.transform(self.config_diff[opt])})

        sql = []
        for option in self.config_diff:
            if re.match('\d+', str(self.config_diff[option])):
                sql.append("SET GLOBAL " + option + "=" + str(self.config_diff[option]) + ";")
            else:
                sql.append("SET GLOBAL " + option + "=\'" + self.config_diff[option] + "\';")

        if self.dryrun:
            self.tools.msg("Options ready to apply online: ", bcolors.OKGREEN)
            for command in sql:
                self.tools.msg(command, bcolors.OKBLUE)
        else:
            self.tools.msg("Apply options online: ", bcolors.PURRPLE)

            if os.path.exists("/root/.my.cnf"):
                try:
                    conn = MySQLdb.connect(host='localhost', read_default_file="/root/.my.cnf")
                    self.tools.msg("Connect to MySQL server success.", bcolors.OKGREEN)
                except MySQLdb.OperationalError as fail:
                    self.tools.msg("MySQL connection failed: " + fail.__str__(), bcolors.FAILRED)
                    return None
            else:
                try:
                    conn = MySQLdb.connect('localhost', 'root', '')
                    self.tools.msg("MySQL connection using no passwd success: ", bcolors.OKGREEN)
                except MySQLdb.OperationalError as fail:
                    self.tools.msg("MySQL connection using no passwd failed: " + fail.__str__(), bcolors.FAILRED)
                    return None

            for command in sql:
                try:
                    conn.query(command)
                except conn.OperationalError as fail:
                    self.tools.msg("Execution statement failed: " + command + " : " + fail[1], bcolors.WARNYELLOW)
                    pass
                except:
                    self.tools.msg("Execution statement failed: " + command, bcolors.WARNYELLOW)
                    pass

    def read_conf_as_dict(self):
        """reads existing mysql config and saves it as dict in self.l_mycnf"""
        self.conffiles = []
        self.find_conffiles('/etc/mysql/conf.d')
        self.conffiles.append('/etc/mysql/my.cnf')
        self.l_mycnf = {}
        for conf in self.conffiles:
            c = self.cnf2dict(StringIO.StringIO(open(conf).read()))
            for key in c:
                if key in self.l_mycnf:
                    self.l_mycnf[key].update(c[key])
                else:
                    self.l_mycnf.update({ key: c[key] })

    def find_conffiles(self, dir):
        if os.path.exists(dir):
            for roots, dirs, files in os.walk(dir):
                for file in fnmatch.filter(files, '*.cnf'):
                    self.conffiles.append(os.path.join(roots, file))

    def get_mycnf(self):
        # get data from conductor that gets it from d.y-t.ru %)
        # main my.cnf
        url = "http://c.yandex-team.ru/api/generator/mysql_config?fqdn=" + self.fqdn
        try:
            _new_data = requests.get(url)
            if _new_data.status_code != 200:
                self.tools.msg("API request " + url + " failed with status " + _new_data.status_code, bcolors.FAILRED)
                return None
            else:
                _newcnf = _new_data.content
                if _newcnf is not None:
                    self.newconfig = StringIO.StringIO(_newcnf)
                    self.newconfig.seek(self.newconfig.len)
                    #self.newconfig.write()
                    self.newconfig.write("!includedir /etc/mysql/conf.d/\n")

        except requests.RequestException as fail:
            self.tools.msg("Conductor API request failed: " + fail.__str__(), bcolors.FAILRED)
            return None

    def cnf2dict(self, conf):
        dictcnf = {}
        #print conf
        for line in conf:
            if re.match('^\[\w+\]$', line):
                secname = line.replace('[', '').replace(']', '').strip()
                dictcnf.update({secname: {}})
            elif re.match('^\w+\s*=?\s*(\w*)?', line):
                opt = line.split('=', 1)
                if len(opt) < 2:
                    dictcnf[secname].update({opt[0].strip(): 1})
                else:
                    dictcnf[secname].update({opt[0].strip(): opt[1].strip()})
        # print dictcnf
        return dictcnf

    def get_clientcnf(self):

        url = "http://c.yandex-team.ru/api/generator/mysql_root_config?fqdn=" + self.fqdn + "&v=3"

        try:
            _client_cnf_data = requests.get(url)
            if _client_cnf_data.status_code != 200:
                self.tools.msg("API request " + url + " failed with status " + _client_cnf_data.status_code,  bcolors.FAILRED)
                _client_cnf = None
            else:
                _client_cnf = ConfigParser.ConfigParser()
                _client_cnf.readfp(StringIO.StringIO(_client_cnf_data.content))
        except requests.RequestException as fail:
            self.tools.msg("Conductor API request failed: " + fail.__str__(), bcolors.FAILRED)
            _client_cnf = None

        # self.clientcnf =  _client_cnf

        # replace autogenerated values to correct values
        for section in _client_cnf.sections():
            if _client_cnf.has_option(section, 'password'):
                if _client_cnf.has_option(section, 'user'):
                    user = _client_cnf.get(section, 'user')
                    if user in self.grants.users:
                        _client_cnf.set(section, 'password', self.grants.users[user])
                    else:
                        self.tools.msg("User %s has no @localhost grants. Skip..." % user, bcolors.WARNYELLOW)

        self.clientcnf = StringIO.StringIO()
        _client_cnf.write(self.clientcnf)

    def get_conf_diff(self):
        """get local config as dict"""
        # convert new config to dict
        newconf =  self.cnf2dict(StringIO.StringIO(self.newconfig.getvalue()))
        #newconf = self.cnf2dict(self.newconfig)
        # read local old config to dict
        self.read_conf_as_dict()
        self.config_diff = {}
        for opt in newconf['mysqld']:
            if opt in self.l_mycnf['mysqld']:
                self.config_diff.update({ opt.replace('-', '_') : newconf['mysqld'][opt] })

    def transform(self, val):
        """transform values like 1G to bytes"""
        if isinstance(val, str):
            if re.match('^[0-9]+(K|M|G)+', val):
                if 'K' in val:
                    val = int(val[:-1]) * 1024
                elif 'M' in val:
                    val = int(val[:-1]) * 1024 * 1024
                elif 'G' in val:
                    val = int(val[:-1]) * 1024 * 1024 * 1024
        return val

    def write_config(self):
        """
        writes all configs to fs: .my.cnf, /etc/mysql/my.cnf, /etc/mysql/client.cnf
        """
        if self.dryrun:
            return None

        with open('/root/.my.cnf', 'w') as conf:
            conf.write(self.dotmycnf.getvalue())
        conf.close()

        with open('/etc/mysql/my.cnf', 'w') as conf:
            conf.write(self.newconfig.getvalue())
        conf.close()

        with open('/etc/mysql/client.cnf', 'w') as conf:
            conf.write(self.clientcnf.getvalue())
        conf.close()

    def define_my_version(self):
        v = re.compile('Ver\s[0-9\.\-]+')
        out = self.tools.run(['mysqld', '-V'])
        v_txt = out.stdout.readlines()
        version = None
        for line in v_txt:
            data = v.findall(line)
            if len(data) > 0:
                try:
                    version = data[0].split()[1]
                except:
                    pass

        return version

    def mysql_check_alive(self):

        if self.l_mycnf is not None:
            pidfile = self.l_mycnf['mysqld']['pid-file']
        else:
            self.read_conf_as_dict()
            pidfile = self.l_mycnf['mysqld']['pid-file']

        if os.path.exists(pidfile):
            pid = open(pidfile).read()
        else:
            return False

        if re.match('\d+', pid):
            if os.path.exists('/proc/'+pid):
                return True
            else:
                return False
        else:
            return False

    def serverctl(self, cmd):
        """starts or stops mysql"""
        # read config

        if self.l_mycnf is not None:
            pidfile = self.l_mycnf['mysqld']['pid-file']
        else:
            self.read_conf_as_dict()
            pidfile = self.l_mycnf['mysqld']['pid-file']

        if cmd == 'stop':
            # check access with mysqladmin

            self.tools.msg('Stopping mysql server...')

            if os.path.exists(pidfile):
                pid = open(pidfile).read()
            else:
                pid = None

            timeout = self.timeout

            if pid is None:
                out = self.tools.run(['pgrep', '-f', pidfile])
                pid =  out.stdout.readline()

            if re.match('\d+', pid):
                self.tools.msg('Stopping mysql server...')
                os.kill(int(pid), 15)
                while timeout > 0:
                    if self.mysql_check_alive():
                        sleep(1)
                        timeout = timeout-1
                    else:
                        self.tools.msg('Mysql server stopped.', bcolors.DEFAULT)
                        return 0

                self.tools.msg('Failed to stop mysql')
                return 1
            else:
                self.tools.msg('Mysql is already stopped.')
                return 0

        elif cmd == 'start':
            self.tools.msg('Starting mysql server...')
            out = self.tools.run(['service', 'mysql', 'start'])
            out.wait()

            """
            # check start
            timeout = self.timeout
            while timeout > 0:
                q = self.mysql_check_alive()
                print q
                if q:
                    self.tools.msg('Mysql started.', bcolors.DEFAULT)
                    return 0
                else:
                    sleep(2)
                    timeout = timeout-2
                    print timeout

            self.tools.msg('Failed to start mysql')
            return 1
            """
            return 0

    def initdatadir(self):
        """do mysql_install_db to new datadir"""

        if self.config_diff is None:
            self.get_conf_diff()

        if 'datadir' in self.config_diff:
            self.tools.msg("Setup new datadir "+self.config_diff['datadir'], bcolors.PURRPLE)
            # workaround for different init datadir in mysql < 5.7 and 5.7+
            # mysql_install_db is deprecated as of MySQL 5.7.6 and --skip-name-resolve was removed in MySQL 5.7.5
            my_version = self.define_my_version()
            if my_version is not None:
                my_version = int(my_version[:3].replace('.',''))

            if my_version > 56:
                cmd = [ 'mysqld', '--initialize-insecure', '--user=mysql']
            else:
                cmd = [ 'mysql_install_db', '--skip-name-resolve', '--user=mysql']

            #self.serverctl('stop')
            #sleep(5)
            self.serverctl('start')

            # self.tools.run(['mysql_install_db', '--datadir='+self.changed_opts['datadir']])
