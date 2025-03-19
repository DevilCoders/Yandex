"""
Mysql client wrapper
"""
import os
import logging
import MySQLdb


MySQLError = MySQLdb.MySQLError         # pylint: disable=C0103,no-member


class MySQL(object):
    """Mysql client wrapper class"""
    def __init__(self):
        self.log = logging.getLogger(self.__class__.__name__)
        self.conn = None

    def is_connected(self):
        """ Returns whether the connection is established """
        return self.conn != None

    def connect(self, host="localhost", read_default_group='client'):
        """ Tries to establish a connection """
        default_file = '/etc/mysql/client.cnf'
        if 'HOME' in os.environ:
            if os.path.exists(os.environ['HOME']+"/.my.cnf"):
                default_file = os.path.expanduser("~/.my.cnf")
        try:
            self.conn = MySQLdb.connect(
                host=host,
                read_default_group=read_default_group,
                read_default_file=default_file,
            )
            self.log.debug('Connected to MySQL')
            return True
        except MySQLError as err:
            self.log.debug('Connection to MySQL(host=%s) failed: %s', host, err)
            return False

    def query(self, query, as_dict=False, raw=False):
        """ Performs an SQL query

        Arguments:
            query {str} -- The query to run

        Keyword Arguments:
            raw {bool} -- Return raw mysql response (default: {False})
            as_dict {bool} -- Convert rows to the dictionary (default: {False})

        Returns:
            list/dict -- The result

        Raises:
            MySQLError -- the MySQLdb error
        """
        try:
            cur = self.conn.cursor()
            cur.execute(query)
            if raw:
                return cur.fetchall()
            elif as_dict:
                return dict(cur.fetchall())
            # todo: take a look on this and no-else-return
            if cur.description is not None: # pylint: disable=no-else-return
                names = [x[0] for x in cur.description]
                return [dict(zip(names, x)) for x in cur.fetchall()]
            else:
                return [dict(zip(x)) for x in cur.fetchall()]
        except MySQLError as err:
            self.log.debug('Got error from MySQL: %s', err)
            raise
