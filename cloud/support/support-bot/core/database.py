#!/usr/bin/env python3
"""This module contains Database class."""

import logging
import pymysql
from pymysql.cursors import DictCursor
from pymysql.err import OperationalError

from utils.decorators import retry
from utils.helpers import make_perfect_dict
from core.constants import BOOLEAN_KEYS

logger = logging.getLogger(__name__)


class Database:
    """This class provides an interface for working with a MySQL database.

    Arguments:
      host: str
      port: int
      db_name: str
      user: str
      passwd: str
      ssl: str - absolute path to CA cert .crt, like a /root/.certs/root.crt
      charset: str - like a 'utf8'

    CA URL: https://storage.yandexcloud.net/cloud-certs/CA.pem

    """

    def __init__(self,
                 host=None,
                 port=None,
                 db_name=None,
                 user=None,
                 passwd=None,
                 ssl=None,
                 use_unicode='true',
                 charset='utf8mb4'):

        self.host = host
        self.port = port
        self.db_name = db_name
        self.user = user
        self.passwd = passwd
        self.charset = charset
        self.use_unicode = use_unicode or 'true'
        self.cusorclass = DictCursor
        self.ssl = ssl

        self.ca = {
            'ca': self.ssl
        }

    def connection(self):
        conn = pymysql.connect(
            host=self.host,
            user=self.user,
            db=self.db_name,
            passwd=self.passwd,
            ssl=self.ca,
            cursorclass=self.cusorclass,
            use_unicode=self.use_unicode,
            charset=self.charset
        )
        return conn

    @retry(OperationalError)
    def query(self, sql, fetchall=False, close=False):
        """A simple method for executing a DB query.

        Args:
          sql: str - sql query.
          fetchall: bool - return many results.
          close: bool - if true, connect will be closed after execute.

        """
        connection = self.connection()
        logger.debug(sql)
        result = None

        try:
            connection.ping(reconnect=True)
            with connection.cursor() as cursor:
                cursor.execute(sql)
                result = cursor.fetchall() if fetchall else cursor.fetchone()
                logger.debug(f'MySQL.result: {result}')
            cursor.close()  # TESTING, DELETE IF THERE ARE PROBLEMS
        except Exception as err:
            logger.error(f'MySQL.query: {sql}, Error: {err}')
        else:
            connection.commit()
        finally:
            connection.close() if close else None
            return result

    # USERS

    def add_user(self, user: object, table='users'):
        """Add User in to database."""
        sql = f'INSERT INTO `{table}` ' + \
              f'(telegram_id, telegram_login, staff_login, is_allowed, is_support) ' + \
              f'VALUES ({user.telegram_id}, "{user.telegram_login}", "{user.staff_login}", ' + \
              f'{user.is_allowed}, 1)'
#              f'{user.is_allowed}, {user.is_support})'

        logger.info(f'Trying to add user {user.telegram_login} (staff: {user.staff_login}) to DB (table: {table}).')
        return self.query(sql, close=True)

    def get_all_users(self, with_config=False, table='users', with_roles=False):
        """Returns records as list of dict, if it exists."""
        sql = f'SELECT * ' + \
              f'FROM `{table}`'

        if with_config and with_roles:
            sql = f'SELECT * ' + \
                f'FROM `{table}` ' + \
                f'LEFT JOIN `users_config` USING (telegram_id)' + \
                f'LEFT JOIN `support_roles` USING (staff_login)'

        elif with_config:
            sql = f'SELECT * ' + \
                f'FROM `{table}` ' + \
                f'LEFT JOIN `users_config` USING (telegram_id)'

        elif with_roles:
            sql = f'SELECT * ' + \
                f'FROM `{table}` ' + \
                f'LEFT JOIN `support_roles` USING (staff_login)'

        result = self.query(sql, fetchall=True, close=True)
        if not result:
            logging.debug(f'Users not found in DB.')
            return False

        users = []
        logger.debug('Prepairing users from database to objects...')
        for user in result:
            users.append(make_perfect_dict(user))
        logger.debug(f'User objects done')
        return users

    def get_user(self, user: object, with_config=False, table='users'):
        """Returns record as dict, if it exists."""
        sql = f'SELECT * ' + \
              f'FROM `{table}` ' + \
              f'WHERE telegram_id = "{user.telegram_id}"'

        if with_config:
            sql = f'SELECT * ' + \
                f'FROM `{table}` ' + \
                f'LEFT JOIN users_config USING (telegram_id) ' + \
                f'WHERE telegram_id = "{user.telegram_id}"'

        result = self.query(sql, close=True)
        if not result:
            logging.debug(f'User {user.telegram_login} not found in DB.')
            return False
        return make_perfect_dict(result)

    def update_user(self, user: object, table='users'):
        """Updating only different values for User in the database."""
        old_data = self.get_user(user)
        for key, value in user.to_dict().items():
            if value != old_data[key]:
                logging.info(f'Updating "{key}" for {user.telegram_login} in DB. Old: {old_data[key]}, New: {value}')
                new_value = value if key in BOOLEAN_KEYS else f'"{value}"'
                sql = f'UPDATE `{table}` ' + \
                      f'SET {key} = {new_value} ' + \
                      f'WHERE telegram_id = "{user.telegram_id}"'

                self.query(sql, close=True)

    def delete_user(self, user: object, table='users'):
        """Delete User from database."""
        sql = f'DELETE FROM {table} ' + \
              f'WHERE telegram_id = {user.telegram_id}'

        logger.info(f'Trying to delete user {user.telegram_login} (staff: {user.staff_login}) from DB (table: {table}).')
        return self.query(sql, close=True)

    # USERCONFIG

    def add_user_config(self, userconfig: object, table='users_config'):
        """Add User in to database."""
        sql = f'INSERT INTO `{table}` ' + \
              f'(telegram_id, is_admin, is_duty, cloudabuse, cloudsupport, ' + \
              f'premium_support, business_support, standard_support, ' + \
              f'ycloud, work_time, do_not_disturb, ignore_work_time) ' + \
              f'VALUES ({userconfig.telegram_id}, {userconfig.is_admin}, {userconfig.is_duty}, ' + \
              f'{userconfig.cloudabuse}, {userconfig.cloudsupport}, {userconfig.premium_support}, '\
              f'{userconfig.business_support}, {userconfig.standard_support}, ' + \
              f'{userconfig.ycloud}, "{userconfig.work_time}", {userconfig.do_not_disturb}, ' + \
              f'{userconfig.ignore_work_time})'

        logger.info(f'Trying to add config for {userconfig.telegram_id} to DB (table: {table}).')
        return self.query(sql, close=True)

    def get_user_config(self, userconfig: object, table='users_config'):
        """Returns record as dict, if it exists."""
        sql = f'SELECT * ' + \
              f'FROM `{table}` ' + \
              f'WHERE telegram_id = "{userconfig.telegram_id}"'

        result = self.query(sql, close=True)
        if not result:
            logging.debug(f'User config for {userconfig.telegram_id} not found in DB.')
            return False
        return make_perfect_dict(result)

    def get_user_role(self, user: object):
        """Returns record as dict, if it exists."""
        sql = f'SELECT * ' + \
              f'FROM `support_roles` ' + \
              f'WHERE `staff_login`="{user.staff_login}"'
#        print(f'SQL__QUERY__:\n' + sql)

        result = self.query(sql, close=True)
        if not result:
            logging.debug(f'Roles for {user.staff_login} in today not found in DB.')
            return False
        return result

    def update_user_config(self, userconfig: object, key: str, value: [str, bool], table='users_config'):
        """Updating only different values for User in the database."""
        old_data = self.get_user_config(userconfig)
        if old_data.get(key) != value:
            logging.info(f'Updating "{key}" for cfg {userconfig.telegram_id} in DB. Old: {old_data[key]}, New: {value}')
            new_value = value if key in BOOLEAN_KEYS else f'"{value}"'
            sql = f'UPDATE `{table}` ' + \
                  f'SET {key} = {new_value} ' + \
                  f'WHERE telegram_id = "{userconfig.telegram_id}"'

            self.query(sql, close=True)

    def delete_user_config(self, userconfig: object, table='users_config'):
        """Delete User from database."""
        sql = f'DELETE FROM {table} ' + \
              f'WHERE telegram_id = {userconfig.telegram_id}'

        logger.info(f'Trying to delete user config for {userconfig.telegram_id} from DB (table: {table}).')
        return self.query(sql, close=True)


def init_db_client():
    """Init and return the database client."""
    from utils.config import Config

    client = Database(
        host=Config.HOST,
        port=Config.PORT,
        db_name=Config.DB_NAME,
        user=Config.USER,
        passwd=Config.PASSWD,
        ssl=Config.CA_PATH
    )
    return client
