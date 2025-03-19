"""
Working with mysql configs
Generates and writes MySQL configs, update grants, init configuration
"""
import stat
import pwd
import re
import os
import logging
from socket import gethostname
from StringIO import StringIO
from subprocess import check_output, CalledProcessError
from time import sleep
from mysql_configurator.grants.grantushka import Grantushka, MysqlWorker
from mysql_configurator.grants.utils import cnf2dict, dict2cnf, transform


def readdir(dirname):
    """
    read files in direectory and appends it into config
    :param dirname: path to directory
    :return: config as StringIO
    """
    conf = StringIO()
    for root, _, files in os.walk(dirname, topdown=False):
        for name in files:
            conf.write(open(os.path.join(root, name)).read())
    return conf.getvalue()


def readconfig(filename):
    """
    read config with all included configs
    :param filename: path to config file
    :return: config as StringIO
    """
    result = StringIO()
    for line in open(filename).readlines():
        if re.match(r'^!include\s', line):
            result.writelines(readconfig(line.split()[1]))
        elif re.match(r'^!includedir\s', line):
            result.write(readdir(line.split()[1]))
        else:
            result.write(line)
    # magic for correct object
    return StringIO(result.getvalue())


class MysqlControl(object):
    """
    Working with MySQL server operations like start stop install_db
    """
    def __init__(self, timeout=300):
        self.log = logging.getLogger(self.__class__.__name__)
        self.mysql = MysqlWorker()
        self.timeout = timeout

    @property
    def is_alive(self):
        """
        check mysql server alive
        :return: bool
        """
        try:
            pid = check_output(['pgrep', '-f', '/usr/sbin/mysqld'])
        except CalledProcessError:
            # if pid not found we guess mysqld is dead
            return False

        if re.match(r'\d+', pid):
            if not os.path.exists('/proc/'+str(pid).strip()):
                return False
        else:
            return False

        # if mysql alive try to ping it
        test = self.mysql.execute("select 1")

        if not test:
            return False

        return True

    def stop(self):
        """
        stop mysql server
        :return: bool
        """
        timeout = self.timeout

        if not self.is_alive:
            self.log.debug('mysqld looks already stopped.')
            return True

        self.log.info('Stopping mysql server...')
        try:
            check_output('/usr/sbin/service mysql stop', shell=True)
        except CalledProcessError as error:
            self.log.warn(error)
            return False

        while timeout > 0:
            if self.is_alive:
                self.log.info('Mysql seems still alive...')
                sleep(1)
                timeout -= 1
            else:
                self.log.info('Mysql server stopped.')
                return True

        self.log.error('Failed to stop mysql')
        return False

    def start(self):
        """
        start mysql server
        :return: bool
        """
        timeout = self.timeout
        self.log.info('Starting mysql server...')
        try:
            check_output(['/usr/sbin/service', 'mysql', 'start'])
        except CalledProcessError as error:
            self.log.warn(error)
            return False
        # waiting for online
        while timeout > 0:
            if not self.is_alive:
                self.log.info('Mysql servers seems still offline...')
                sleep(1)
                timeout -= 1
            else:
                self.log.info('Mysql server started.')
                return True
        self.log.warn('Timeout exceeded. Mysql start failed.')
        return False

    def init_datadir(self, datadir):
        """
        init data directory(mysql_install_db)
        :param datadir: path to data directory
        :return: None
        """
        if os.path.exists(datadir):
            self.log.debug("Data directory is already exists. Init not needed.")
            if not self.is_alive:
                self.start()
            return
        self.log.info("Setup new data directory %s", datadir)

        version = int(self.mysql.get_version.replace('.', ''))
        if version > 56:
            initial_command = ['/usr/sbin/mysqld',
                               '--initialize-insecure',
                               '--user=mysql',
                               '--datadir=' + datadir]
        else:
            initial_command = ['/usr/sbin/mysql_install_db',
                               '--skip-name-resolve',
                               '--user=mysql',
                               '--datadir=' + datadir]
        if self.is_alive:
            self.log.info("Can't setup new data directory with mysql server started.")
            return
        try:
            check_output(initial_command)
        except CalledProcessError as error:
            self.log.debug(error)


class ConfigWorker(object):
    """
    Working with mysql configs: get, write, configure, apply
    """
    def __init__(self, config):
        self.log = logging.getLogger(self.__class__.__name__)
        self.config = config
        self.grantushka = Grantushka(config)
        self.grantushka.confworker.set_ignore_errors()
        self.fqdn = gethostname()

    def init_grants(self, skip_failed=False):
        """
        Prepare grants for detect users, which will be used for generating /etc/mysq/client.cnf
        :param skip_failed: set ignore errors or not
        """
        if skip_failed:
            self.grantushka.confworker.set_ignore_errors()
        else:
            self.grantushka.confworker.unset_ignore_errors()
        self.grantushka.processhosts()
        self.grantushka.generatemyusers()
        self.grantushka.generate_sql()

    def get_mycnf(self):
        """
        get data from conductor that gets it from d.y-t.ru %)
        generates main my.cnf
        """
        url = "http://c.yandex-team.ru/api/generator/mysql_config?fqdn=" + self.fqdn
        data = self.grantushka.confworker.request(url)
        mainconf = None
        if data:
            mainconf = StringIO(data)
            mainconf.seek(mainconf.len)
            mainconf.write("!includedir /etc/mysql/conf.d/\n")
        else:
            self.log.debug("Failed to get my.cnf from API.")

        return mainconf

    def get_clientcnf(self):
        """
        generates client.cnf and replaces username and password with corrects
        """

        url = "http://c.yandex-team.ru/api/generator/mysql_root_config?fqdn=" + self.fqdn + "&v=3"

        data = self.grantushka.confworker.request(url)
        client_conf = None
        if data:
            client_conf = StringIO(data)
        else:
            self.log.error("Failed to get client config from API")

        # replace passwords
        client_conf = cnf2dict(client_conf)
        users = self.grantushka.conf['users']
        for section in client_conf:
            if 'password' in client_conf[section].keys():
                user = client_conf[section]['user'] or ''
                password = users[user] if user in users else ''
                client_conf[section].update({'password': password})

        client_conf = dict2cnf(client_conf)
        return client_conf

    @staticmethod
    def read_config(as_dict=True):
        """
        read current mysql config file
        :param as_dict: convert to dict or stay raw
        :return: config
        """
        config = readconfig('/etc/mysql/my.cnf')
        if not as_dict:
            return config.getvalue()

        return cnf2dict(config)

    @staticmethod
    def get_modified(one, two):
        """
        get options which must be added or modified
        :param one: old config as dictionary
        :param two: new config as dictionary
        :return: new and changed options as dictionary
        """
        if isinstance(one, StringIO):
            one = cnf2dict(StringIO(one.getvalue()))
        if isinstance(two, StringIO):
            two = cnf2dict(StringIO(two.getvalue()))

        # we interested only in mysqld section
        try:
            current = one['mysqld']
        except KeyError:
            current = {}
        try:
            new = two['mysqld']
        except KeyError:
            new = {}

        result = {}

        new_options = set(new.keys()).difference(set(current.keys()))
        changed_options = set(new.keys()).intersection(set(current.keys()))
        # add new
        for option in new_options:
            result.update({option: new[option]})
        # rewrite old
        for option in changed_options:
            result.update({option: new[option]})

        return result

    @staticmethod
    def rewrite_otion_names(options):
        """
        convert config options into mysql variables to apply online
        :param options: options list
        :return: variables list
        """
        keys = options.keys()
        result = {}
        for key in keys:
            newkey = key.replace('-', '_')
            result.update({newkey: options[key]})
        return result

    @ staticmethod
    def sql_set(option, value):
        """
        generate mysql set expression
        :param option: variable to set
        :param value: value to set
        :return: set mysql command
        """
        # convert vriables like 10MB to digital
        if re.match(r'^[0-9]+[KMG]$', str(value)):
            value = transform(value)
        if re.match(r'\d+', str(value)):
            command = "SET GLOBAL {}={};".format(option, value)
        else:
            command = "SET GLOBAL {}=\'{}\';".format(option, value)
        return command

    def generate_new_settings(self, new_config):
        """
        generate settings which differs in new config and should be applied
        :param new_config: new main config
        :return: sql command list
        """
        sql = []
        old_config = self.read_config(as_dict=True)
        new_options = self.get_modified(old_config, new_config)
        new_options = self.rewrite_otion_names(new_options)

        for option in new_options:
            data = self.grantushka.myworker.execute("select @@{}".format(option))
            if data:
                current_option = data[0]["@@{}".format(option)]
                if current_option != new_options[option]:
                    sql.append(self.sql_set(option, new_options[option]))

        return sql

    def apply_new_settings(self, sql):
        """
        apply command list
        :param sql: sql commands list
        :return: None
        """
        for command in sql:
            self.grantushka.myworker.execute(command)

    def save(self, filename, data):
        """
        Just saves config file(expect StringIO text object)
        :param filename: path to config
        :param data: config data
        :return: None
        """
        try:
            conf = open(filename, 'w')
            conf.write(data)
        except OSError as error:
            self.log.error(error)
            return

    def save_client(self, config_data):
        """
        save both client config and .my.cnf
        :param config_data: client config data
        :return: None
        """
        client_config_path = '/etc/mysql/client.cnf'
        root_config_path = '/root/.my.cnf'
        self.save(root_config_path, "!include /etc/mysql/client.cnf\n")
        self.save(client_config_path, config_data)
        # strict permissions for client.cnf
        os.chmod(client_config_path, stat.S_IRUSR | stat.S_IRGRP)
        uid = 0
        try:
            uid = pwd.getpwnam("mysqladmin").pw_uid
        except KeyError:
            self.log.debug("User mysqladmin not found, using default ownership for client config")
        os.chown(client_config_path, uid, 0)

    def init_configuration(self):
        """
        initial mysql configuration with replace configs, update grants, init data directory
        :return: None
        """
        main_config_path = '/etc/mysql/my.cnf'
        client_config_path = '/etc/mysql/client.cnf'
        root_config_path = '/root/.my.cnf'
        if self.config.dryrun:
            self.log.info("Initial configuration can't be run in dyrun mode.")
            return

        new_main_config = self.get_mycnf()
        if not new_main_config:
            self.log.warn("Can't continue initial configuration without new config.")
            return
        old_main_config = self.read_config()
        options_to_apply = self.get_modified(old_main_config, new_main_config)
        client_config = self.get_clientcnf()

        if 'datadir' in options_to_apply.keys():
            datadir = options_to_apply['datadir']
            if not os.path.exists(datadir):
                # init new data directory and start mysql with new config
                myctl = MysqlControl()
                myctl.stop()
                myctl.init_datadir(datadir)
                self.log.info("Save new main config.")
                self.save(main_config_path, new_main_config.getvalue())
                myctl.start()
            else:
                # only save new config
                self.log.info("Data directory exists. Init data skipped.")
                self.log.info("Save new main config.")
                self.save(main_config_path, new_main_config.getvalue())
            # the following step must be executed anyway
            self.log.debug("Prepare to update grants.")
            self.init_grants(skip_failed=True)
            self.grantushka.update()
            self.log.info("Save client config.")
            self.save(client_config_path, client_config.getvalue())
            self.log.info("Save user config.")
            self.save(root_config_path, "!include /etc/mysql/client.cnf\n")
