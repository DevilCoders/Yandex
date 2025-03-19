# -*- coding: utf-8 -*-
"""
Module to provide SQLServer compatibility to salt.

:depends:   - ODBC Driver
            - pyodbc Python module

"""

# Import python libs
from __future__ import absolute_import, print_function, unicode_literals
import ConfigParser
import logging
import os
import salt.ext.six as six
import time
import subprocess
import json

try:
    import pyodbc

    HAS_ALL_IMPORTS = True
except ImportError:
    HAS_ALL_IMPORTS = False

log = logging.getLogger(__name__)

PV = {
    'backup_preference': ['PRIMARY', 'SECONDARY_ONLY', 'SECONDARY', 'NONE'],
    'failure_condition_level': [1, 2, 3, 4, 5],
    'db_failover': ['ON', 'OFF'],
    'av_mode': ['SYNCHRONOUS_COMMIT', 'ASYNCHRONOUS_COMMIT', 'CONFIGURATION_ONLY'],
    'fo_mode': ['AUTOMATIC', 'MANUAL', 'EXTERNAL'],
    'seeding_mode': ['AUTOMATIC', 'MANUAL'],
    'sec_allow_connections': ['NO', 'READ_ONLY', 'ALL'],
}

ALL_REGISTRY_OPTIONS = [
    ['AuditLevel', '', 'REG_DWORD'],
    ['BackupDirectory', '', 'REG_SZ'],
    ['LoginMode', '', 'REG_DWORD'],
    ['EnableLevel', 'Filestream', 'REG_DWORD'],
    ['ShareName', 'Filestream', 'REG_SZ'],
    ['HADR_Enabled', 'HADR', 'REG_DWORD'],
    ['Certificate', 'SuperSocketNetLib', 'REG_SZ'],
    ['ForceEncryption', 'SuperSocketNetLib', 'REG_DWORD'],
    ['TcpDynamicPorts', 'SuperSocketNetLib\Tcp\IPAll', 'REG_SZ'],
    ['TcpPort', 'SuperSocketNetLib\Tcp\IPAll', 'REG_SZ'],
]
REGISTRY_OPTIONS = [r[0] for r in ALL_REGISTRY_OPTIONS]

REGISTRY_OPTIONS_TYPES = {r[0]: r[2] for r in ALL_REGISTRY_OPTIONS}

REGISTRY_OPTIONS_PATH = {r[0]: r[1] for r in ALL_REGISTRY_OPTIONS}


SERVER_ROLES = [
    'sysadmin',
    'securityadmin',
    'serveradmin',
    'setupadmin',
    'processadmin',
    'diskadmin',
    'dbcreator',
    'bulkadmin',
]

# Priviledges. The second value in a row is a provoledge that implies the first one
# TODO: Need to add roles, that impliy privs.
# If the one has the role it is impossible to give a priv that is included in the role.
SERVER_PRIVS = [
    ['ADMINISTER BULK OPERATIONS', 'CONTROL SERVER'],
    ['ALTER ANY AVAILABILITY GROUP', 'CONTROL SERVER'],
    ['ALTER ANY CONNECTION', 'CONTROL SERVER'],
    ['ALTER ANY CREDENTIAL', 'CONTROL SERVER'],
    ['ALTER ANY DATABASE', 'CONTROL SERVER'],
    ['ALTER ANY ENDPOINT', 'CONTROL SERVER'],
    ['ALTER ANY EVENT NOTIFICATION', 'CONTROL SERVER'],
    ['ALTER ANY EVENT SESSION', 'CONTROL SERVER'],
    ['ALTER ANY LINKED SERVER', 'CONTROL SERVER'],
    ['ALTER ANY LOGIN', 'CONTROL SERVER'],
    ['ALTER ANY SERVER AUDIT', 'CONTROL SERVER'],
    ['ALTER ANY SERVER ROLE', 'CONTROL SERVER'],
    ['ALTER RESOURCES', 'CONTROL SERVER'],
    ['ALTER SERVER STATE', 'CONTROL SERVER'],
    ['ALTER SETTINGS', 'CONTROL SERVER'],
    ['ALTER TRACE', 'CONTROL SERVER'],
    ['AUTHENTICATE SERVER', 'CONTROL SERVER'],
    ['CONNECT ANY DATABASE', 'CONTROL SERVER'],
    ['CONNECT SQL', 'CONTROL SERVER'],
    ['CONTROL SERVER', 'CONTROL SERVER'],
    ['CREATE ANY DATABASE', 'ALTER ANY DATABASE'],
    ['CREATE AVAILABILITY GROUP', 'ALTER ANY AVAILABILITY GROUP'],
    ['CREATE DDL EVENT NOTIFICATION', 'ALTER ANY EVENT NOTIFICATION'],
    ['CREATE ENDPOINT', 'ALTER ANY ENDPOINT'],
    ['CREATE SERVER ROLE', 'ALTER ANY SERVER ROLE'],
    ['CREATE TRACE EVENT NOTIFICATION', 'ALTER ANY EVENT NOTIFICATION'],
    ['EXTERNAL ACCESS ASSEMBLY', 'CONTROL SERVER'],
    ['IMPERSONATE ANY LOGIN', 'CONTROL SERVER'],
    ['SELECT ALL USER SECURABLES', 'CONTROL SERVER'],
    ['SHUTDOWN', 'CONTROL SERVER'],
    ['UNSAFE ASSEMBLY', 'CONTROL SERVER'],
    ['VIEW ANY DATABASE', 'VIEW ANY DEFINITION'],
    ['VIEW ANY DEFINITION', 'CONTROL SERVER'],
    ['VIEW SERVER STATE', 'ALTER SERVER STATE'],
]


def __virtual__():
    """
    Only load this module if all imports succeeded
    """
    if HAS_ALL_IMPORTS:
        return True
    return (False, 'The sqlserver execution module cannot be loaded: the pyodbc python library is not available.')


def _get_connection(**kwargs):
    dbname = kwargs.get('database', 'master')
    opts = {
        'Driver': '{ODBC Driver 17 for SQL Server}',
        'Database': 'master',
        'Server': '(local)',
        'Trusted_Connection': 'Yes',
    }
    if 'database' in kwargs:
        opts['Database'] = kwargs['database']
    if 'uid' in kwargs:
        opts['Uid'] = kwargs.get('uid')
        opts['Pwd'] = kwargs.get('pwd', '')
        opts['Server'] = 'localhost,1433'
    connstr = ';'.join(k + '=' + v for k, v in opts.items())
    conn = pyodbc.connect(connstr)
    conn.autocommit = True
    return conn


def _escape_obj_name(var):
    var = var.replace("]", "]]")
    var = '[' + var + ']'
    return var


def _escape_string(var):
    var = var.replace("'", "''")
    var = "'" + var + "'"
    return var


def run_query(sql, **kwargs):
    """
    Run a t-sql query, returning a result.

    """
    conn = None
    try:
        conn = get_connection(**kwargs)
        return [row for row in conn.cursor().execute(sql).fetchall()]
    finally:
        if conn is not None:
            conn.close()


def get_connection(**kwargs):
    return _get_connection(**kwargs)


def user_role_list(username, database, domain=None, **kwargs):
    """
    lists out the database roles the user is member of.

     CLI Example:

     .. code-block:: bash

         salt minion mdb_sqlserver.user_role_list USERNAME database=DBNAME
    """
    if 'database' not in kwargs:
        kwargs['database'] = database
    cur = _get_connection(**kwargs).cursor()
    query = '''
    SELECT dp.name
        FROM sys.database_principals dp
        JOIN sys.database_role_members rm
            ON dp.principal_id = rm.role_principal_id
        JOIN sys.database_principals dp2
            ON dp2.principal_id = rm.member_principal_id
    WHERE dp2.name = ?;
    '''
    cur.execute(query, username)
    return [row[0] for row in cur.fetchall()]


def user_exists(username, database, domain=None, **kwargs):
    """
    Find if a user exists in a specific database on the SQLServer.
    domain, if provided, will be prepended to username

    CLI Example:

    .. code-block:: bash

        salt minion mdb_sqlserver.user_exists 'USERNAME', database='DBNAME', [domain = MYDOMAIN]
    """
    if domain and not '\\' in username:
        username = '{0}\\{1}'.format(domain, username)
    if database:
        kwargs['database'] = database

    cnxn = _get_connection(**kwargs)

    cur = cnxn.cursor()
    query = '''
        SELECT
                1
            FROM sys.database_principals
        WHERE name = ? and type in ('U','S');'''
    cur.execute(query, username)
    return len(cur.fetchall()) == 1


def user_role_mod(username, domain, database, roles_add, roles_drop, **kwargs):
    """
    Modifies user role membership adding user in all roles listed in roles_add and dropping from all roles listed in roles_drop

    CLI Example:

    .. code-block:: bash

        salt minion mdb_sqlserver.user_role_mod USERNAME database=DBNAME roles_add=['db_datawriter','db_datareader']
    """
    if 'database' not in kwargs:
        kwargs['database'] = database
    try:
        cnxn = _get_connection(**kwargs)

        cur = cnxn.cursor()
        # if not a lists has been passed then we'll try to convert those to lists
        if type(roles_add) is not list and len(roles_add) > 0:
            roles_add = [roles_add]
        if type(roles_drop) is not list and len(roles_drop) > 0:
            roles_drop = [roles_drop]
        query = ''
        # build command to add user to all roles needed.
        if len(roles_add) > 0:
            for rl in roles_add:
                query += '''ALTER ROLE {0} ADD MEMBER {1};
'''.format(
                    _escape_obj_name(rl), _escape_obj_name(username)
                )
        if len(roles_drop) > 0:
            for rl in roles_drop:
                query += '''ALTER ROLE {0} DROP MEMBER {1};
'''.format(
                    _escape_obj_name(rl), _escape_obj_name(username)
                )

        cur.execute(query)
        return True
    except pyodbc.OperationalError as err:
        log.error('Could not modify role membership: {0}'.format(err))
        log.exception(err)
        return False


def user_create(
    new_username, new_login=None, new_domain='', new_database=None, new_roles=None, new_options=None, **kwargs
):
    """
    Creates a new user.  If login is not specified, the user will be created
    without a login.  domain, if provided, will be prepended to username.
    options can only be a list of strings

    CLI Example:

    .. code-block:: bash

        salt minion mdb_sqlserver.user_create USERNAME database=DBNAME
    """
    if new_domain and not new_login:
        return False
    if user_exists(new_username, new_domain, new_database, **kwargs):
        return False

    if new_domain:
        new_username = '{0}\\{1}'.format(new_domain, new_username)
        new_login = '{0}\\{1}'.format(new_domain, new_login) if new_login else new_login
    if new_database:
        kwargs['database'] = new_database
    if not new_roles:
        new_roles = []
    if not new_options:
        new_options = []

    sql = "CREATE USER {0} ".format(_escape_obj_name(new_username))
    if new_login:
        # If the login does not exist, user creation will throw
        # if not login_exists(name, **kwargs):
        #     return False
        sql += " FOR LOGIN {0}".format(_escape_obj_name(new_login))
    else:
        sql += " WITHOUT LOGIN"
    if new_options:
        sql += ' WITH ' + ', '.join(
            new_options
        )  # sql injection possible here, but the variety of options is huge, especially password.
    sql += ';'
    conn = None
    try:
        conn = _get_connection(**kwargs)
        conn.cursor().execute(sql)
        for role in new_roles:
            conn.cursor().execute(
                'ALTER ROLE {0} ADD MEMBER {1};'.format(_escape_obj_name(role), _escape_obj_name(new_username))
            )
            return True
    except pyodbc.OperationalError as err:
        log.error('Could not create the user: {0}'.format(err))
        log.exception(err)
        return False


def user_drop(username, database, **kwargs):
    """
    Drop scpecific user in a database.
       All powned schemas and roles will be handed over to dbo.
    """
    if not user_exists(username, database, **kwargs):
        return False
    kwargs['database'] = database
    '''hereby I use two different approaches for passing the same parameter to a query
    it's because it is used in a query were quotation marks are inappropriate and in a
    DROP USER statement, where it is appropriate.
    By combination of these two methods I reduce the possibility of SQL Injection.
    '''
    sql = '''
    SET NOCOUNT ON;

    DECLARE @user SYSNAME = ?;
    DECLARE @objects TABLE (
        obj_name NVARCHAR(255)
        , obj_id INT
        , obj_type INT
        );
    DECLARE @obj_name SYSNAME
        , @obj_type INT;
    DECLARE @sql NVARCHAR(max);

    --schemas and roles to change authorization
    INSERT @objects (
        obj_name
        , obj_id
        , obj_type
        )
    SELECT s.name
        , s.schema_id
        , 1
    FROM sys.schemas s
    WHERE s.principal_id = DATABASE_PRINCIPAL_ID(@user)

    UNION ALL

    SELECT name
        , principal_id
        , 2
    FROM sys.database_principals
    WHERE type_desc = 'DATABASE_ROLE'
        AND owning_principal_id = DATABASE_PRINCIPAL_ID(@user);

    DECLARE Objects CURSOR
    FOR
    SELECT obj_name
        , obj_type
    FROM @objects;

    OPEN Objects;

    FETCH NEXT
    FROM Objects
    INTO @obj_name
        , @obj_type;

    WHILE @@FETCH_STATUS = 0
    BEGIN
        SET @sql = 'ALTER AUTHORIZATION ON ' + CASE @obj_type
                WHEN 1
                    THEN 'SCHEMA'
                WHEN 2
                    THEN 'ROLE'
                END + '::' + QUOTENAME(@obj_name) + ' TO [dbo];'

        EXEC (@sql);

        FETCH NEXT
        FROM Objects
        INTO @obj_name
            , @obj_type;
    END

    CLOSE Objects;

    DEALLOCATE Objects;

    DECLARE @sql2 NVARCHAR(MAX) = N'';

    SELECT @sql2 += N'KILL ' + CONVERT(VARCHAR(11), session_id) + N';'
        FROM sys.dm_exec_sessions
        WHERE
            login_name = @user
            AND session_id <> @@SPID
            AND database_id = DB_ID({db_name})

    EXEC sys.sp_executesql @sql2;

    DROP USER {username} ;
    '''.format(
        username=_escape_obj_name(username), db_name=_escape_string(database)
    )
    try:
        conn = _get_connection(**kwargs)
        conn.cursor().execute(sql, username)
        return True
    except pyodbc.OperationalError as err:
        log.error('Could not drop the user: {0}'.format(err))
        log.exception(err)
        return False


def login_property_get(login, property, **kwargs):
    """
    Extracts one login property value.
    Possible properties to extract:
    IsSQLLogin - Is a sql server login, as opposed to windows login
    BadPasswordCount - Number of consecutive attempts to log in with an incorrect password.
    BadPasswordTime - Time of the last attempt to log in with an incorrect password.
    DaysUntilExpiration - Time number of days until the password expires.
    DefaultDatabase - SQL Server login default database as stored in metadata or master if no database is specified.  NULL for non-SQL Server provisioned users (for example, Windows authenticated users).
    DefaultLanguage - Login default language as stored in metadata.  NULL for non-SQL Server provisioned users, for example, Windows authenticated users.
    HistoryLength - Number of passwords tracked for the login, using the password-policy enforcement mechanism. 0 if the password policy is not enforced. Resuming password policy enforcement restarts at 1.
    IsExpired - Indicates whether the login has expired.
    IsLocked - Indicates whether the login is locked.
    IsMustChange -Indicates whether the login must change its password the next time it connects.
    LockoutTime - the date when the SQL Server login was locked out because it had exceeded the permitted number of failed login attempts.
    PasswordHash - the hash of the password.
    PasswordLastSetTime - the date when the current password was set.
    PasswordHashAlgorithm - the algorithm used to hash the password.

    LoginSID - SID of the ServerPrincipal
    IsSysadmin - Is the login a member of sysadmin role
    """
    sql = ''
    if not property:
        log.error("login_property_get: No property specified!")
        return False
    if not login:
        log.error("login_property_get: No login specified")
        return False
    if property in ('PasswordHash'):
        sql = "SELECT sys.fn_varbintohexstr(CAST(LOGINPROPERTY(?,?) as VARBINARY));"
    if property in (
        'BadPasswordCount',
        'BadPasswordTime',
        'DaysUntilExpiration',
        'DefaultDatabase',
        'HistoryLength',
        'IsExpired',
        'IsLocked',
        'IsMustChange',
        'LockoutTime',
        'PasswordLastSetTime',
        'PasswordHashAlgorithm',
    ):
        sql = "SELECT CAST(LOGINPROPERTY(?,?) as NVARCHAR(MAX));"
    if property == 'IsSQLLogin':
        sql = '''       SELECT
                CASE WHEN type = 'S' THEN 1
                ELSE 0 END as IsSQLLogin
                FROM sys.server_principals
                WHERE
                    name = ?
                    OR (? = '')
    '''
    if property == 'LoginSID':
        sql = "SELECT sys.fn_varbintohexstr(SID) FROM sys.server_principals WHERE name = ? OR (? = '') "
    if property == 'IsSysadmin':
        sql = "SELECT IS_SRVROLEMEMBER('sysadmin',?),?"
    try:
        conn = _get_connection(**kwargs)
        cur = conn.cursor().execute(sql, (login, property))
        rows = cur.fetchall()
        ret = [row[0] for row in rows]
        return ret[0]

    except pyodbc.OperationalError as err:
        log.error('Could not get login property: {0}'.format(err))
        log.exception(err)
        return False


def login_exists(login, domain='', **kwargs):
    """
    Find if a login exists in the MS SQL server.
    domain, if provided, will be prepended to login

    CLI Example:

    .. code-block:: bash

        salt minion mssql.login_exists 'LOGIN'
    """
    if domain:
        login = '{0}\\{1}'.format(domain, login)
    sql = '''SELECT COUNT(*)
            FROM sys.server_principals
            WHERE name = ?'''
    try:
        conn = _get_connection(**kwargs)
        cur = conn.cursor().execute(sql, (login))
        rows = cur.fetchall()
        ret = [row[0] for row in rows]
        if ret[0] > 0:
            ret[0] = True
        else:
            ret[0] = False
        return ret[0]

    except pyodbc.OperationalError as err:
        log.error('Could not check if the login exists: {0}'.format(err))
        log.exception(err)
        return False


def login_password_match(login, password, **kwargs):
    """
    Checks if the password of SQL Login matches the provided one
    """
    if login_property_get(login, 'IsSQLLogin', **kwargs) == 0:
        raise Exception("Login is not a SQL login, can't check password")

    sql = "SELECT PWDCOMPARE(?,password_hash) FROM sys.sql_logins WHERE name like ? "
    try:
        conn = _get_connection(**kwargs)
        cur = conn.cursor().execute(sql, (password, login))
        # rows = cur.fetchall()
        # ret = [row[0] for row in rows]
        return bool(cur.fetchone()[0])

    except pyodbc.OperationalError as err:
        log.error('Could not compare password: {0}'.format(err))
        log.exception(err)
        return False


def login_password_set(login, password, **kwargs):
    """
    Module sets the password for a specific sql server login
    """
    sql = "ALTER LOGIN {0} WITH PASSWORD=N{1}".format(_escape_obj_name(login), _escape_string(password))
    try:
        conn = _get_connection(**kwargs)
        conn.cursor().execute(sql)
        return True
    except pyodbc.OperationalError as err:
        log.error('Could not set the password: {0}'.format(err))
        log.exception(err)
        return False


def login_role_list(login, **kwargs):
    """
    lists out the database roles the user is member of.

     CLI Example:

     .. code-block:: bash

         salt minion mdb_sqlserver.login_role_list login
    """

    cur = _get_connection(**kwargs).cursor()
    query = '''
    SELECT p.name as rolename
        FROM sys.server_principals p
        JOIN sys.server_role_members rm
            ON p.principal_id = rm.role_principal_id
        JOIN sys.server_principals p2
            ON p2.principal_id = rm.member_principal_id
    WHERE p2.name = ?;
    '''

    cur.execute(query, (login))

    roles = cur.fetchall()
    rls = [row[0] for row in roles]
    return rls


def login_create(
    login, new_login_password=None, new_login_domain='', new_login_roles=None, new_login_options=None, **kwargs
):
    """
    TODO: Rewrite completely. Only quick fixes applied.

    Creates a new login.  Does not update password of existing logins.  For
    Windows authentication, provide ``new_login_domain``.  For SQL Server
    authentication, prvide ``new_login_password``.  Since hashed passwords are
    *varbinary* values, if the ``new_login_password`` is 'int / long', it will
    be considered to be HASHED.

    new_login_roles
        a list of SERVER roles

    new_login_options
        a list of strings

    CLI Example:

    .. code-block:: bash

        salt minion mssql.login_create LOGIN_NAME database=DBNAME [new_login_password=PASSWORD]
    """
    # One and only one of password and domain should be specified
    if bool(new_login_password) == bool(new_login_domain):
        return False
    if login_exists(login, new_login_domain, **kwargs):
        return False
    if new_login_domain:
        login = '{0}\\{1}'.format(new_login_domain, login)

    if not new_login_roles:
        new_login_roles = []
    else:
        new_login_roles = list(new_login_roles)
    if not new_login_options:
        new_login_options = []
    else:
        new_login_options = list(new_login_options)
    pwd = ''
    sql = "CREATE LOGIN {0} ".format(_escape_obj_name(login))
    if new_login_domain:
        sql += " FROM WINDOWS "
    elif isinstance(new_login_password, six.integer_types):
        pwd = "PASSWORD=0x{0:x} HASHED,".format(new_login_password)
    else:  # Plain text password
        pwd = "PASSWORD=N{0},".format(_escape_string(new_login_password))
    if new_login_options:
        sql += ' WITH ' + pwd + ', '.join(new_login_options)
    conn = None

    try:
        conn = _get_connection(**kwargs)
        conn.cursor().execute(sql)
        for role in new_login_roles:
            conn.cursor().execute(
                'ALTER SERVER ROLE {0} ADD MEMBER {1};'.format(_escape_obj_name(role), _escape_obj_name(login))
            )
    except pyodbc.OperationalError as err:
        log.error('Could not create login or set the roles membership: {0}; Faulty SQL statement {1}'.format(err, sql))
        log.exception(err)
        return False
    return True


def principal_role_mod(principal, is_login, roles_add, roles_drop, database=None, **kwargs):
    """
    Modifies principal (login or user) role membership adding user in all roles listed in roles_add and dropping from all roles listed in roles_drop

    Parameters:

        principal - name of a login or a database user to alter role membership

        is_login - show what's been passed as a principal - login (True) or user (False)

        roles_add - list of roles to be added

        roles_drop - list of roles to be dropped

        database - name of database where the user resides if we deal with a user, not a login

    CLI Example:

    .. code-block:: bash

        salt minion mdb_sqlserver.login_role_mod vasya, is_login = True roles_add=['someserverrole1','someserverrole2']
    """
    if not is_login and not database:
        raise Exception("Database parameter is required when is_login = False")

    if database and 'database' not in kwargs and not is_login:
        kwargs['database'] = database
    try:
        cnxn = _get_connection(**kwargs)

        cur = cnxn.cursor()
        # if not a lists has been passed then we'll try to convert those to lists
        if type(roles_add) is not list and roles_add:
            roles_add = [roles_add]
        if type(roles_drop) is not list and roles_drop:
            roles_drop = [roles_drop]
        query = ''
        server_key = ''
        if is_login:
            server_key = ' SERVER '

        # build command to add principal to all roles needed.
        if roles_add:
            for rl in roles_add:
                query += '''ALTER {0} ROLE {1} ADD MEMBER {2};
'''.format(
                    server_key, _escape_obj_name(rl), _escape_obj_name(principal)
                )
        if len(roles_drop) > 0:
            for rl in roles_drop:
                query += '''ALTER {0} ROLE {1} DROP MEMBER {2};
'''.format(
                    server_key, _escape_obj_name(rl), _escape_obj_name(principal)
                )

        cur.execute(query)
        return True
    except pyodbc.OperationalError as err:
        log.error('Could not modify role membership: {0}'.format(err))
        log.exception(err)
        return False


def spconfigure_get(**kwargs):

    sql = '''
    SET NOCOUNT ON
    DECLARE @results TABLE (name NVARCHAR(255), value NVARCHAR(255))
    '''

    vals = []
    for opt, val in REGISTRY_OPTIONS_PATH.items():
        sql += '''INSERT @results
        EXEC xp_instance_regread 'HKEY_LOCAL_MACHINE',? ,?
        '''
        vals.append(r"Software\Microsoft\MSSQLServer\MSSQLServer\{0}".format(val))
        vals.append(opt)

    sql += '''
    SELECT name, value FROM @results
    UNION ALL
    SELECT cast(name as NVARCHAR(255)) name,
    CAST(value as NVARCHAR(255)) value
    FROM sys.configurations
    '''

    try:
        conn = _get_connection(**kwargs)
        cur = conn.cursor()
        cur.execute(sql, vals)
        result = cur.fetchall()
        return result
    except pyodbc.OperationalError as err:
        log.error('Could not get server configuration: {0}'.format(err))
        log.exception(err)
        return False


def spconfigure_set(config, **kwargs):
    """
    sets the sp_configure options for sql server. config is a dict variable.
    all options are configured in one batch
    """
    # when we do multistatement batches we need to disable row counter
    sql = 'SET NOCOUNT ON \n'

    # before changing something in configuration we need to take a red pill.
    advanced_options = '''exec sp_configure 'show advanced options','1';
    RECONFIGURE
    '''
    # close the door to wonderland
    advanced_options_off = '''exec sp_configure 'show advanced options','0';
        RECONFIGURE
        '''
    # options that are changed in registry
    reg_opts = {}

    # options that are changed in sp_configure - the default variant if we have no idea how to handle the option
    spconfigure_opts = {}
    # values to be substituted into commands during execution
    vals = []
    for opt, val in config.items():
        if opt in REGISTRY_OPTIONS:
            reg_opts[opt] = val
        else:
            spconfigure_opts[opt] = val

    for opt, val in spconfigure_opts.items():
        # create a command template with placeholders
        # and a list of values to be passed to execute() method
        sql += "exec sp_configure ?, ?\n"
        vals.append(opt)
        vals.append(val)

    for opt, val in reg_opts.items():
        # create a command template with placeholders
        # and a list of values to be passed to execute() method
        sql += "EXEC xp_instance_regwrite 'HKEY_LOCAL_MACHINE',? ,? ,{0},? \n".format(REGISTRY_OPTIONS_TYPES[opt])
        vals.append(r"Software\Microsoft\MSSQLServer\MSSQLServer\{0}".format(REGISTRY_OPTIONS_PATH[opt]))
        vals.append(opt)
        vals.append(val)

    try:
        conn = _get_connection(**kwargs)
        cur = conn.cursor()
        cur.execute(advanced_options)
        cur.execute(sql, vals)
        cur.execute(advanced_options_off)
        return True
    except pyodbc.OperationalError as err:
        log.error('Could not change server configuration: {0}'.format(err))
        log.exception(err)
        return False


def is_replica(**kwargs):
    """
    Returns true if current node is replica
    """
    if '__sqlserver_is_replica' not in __pillar__:
        try:
            conn = _get_connection(**kwargs)
            # detect replica using sql
            # returning values are NULL - not a member of AG, 0 - resolving, 1 - primary, 2 - replica
            sql = """
                SELECT count(*) AS is_replica
                FROM sys.dm_hadr_availability_replica_states
                WHERE is_local = 1 and role in (0,2)
                """
            cur = conn.cursor()
            cur.execute(sql)
            result = cur.fetchone()[0]
            __pillar__['__sqlserver_is_replica'] = result > 0
        except (pyodbc.OperationalError, pyodbc.InterfaceError) as err:
            log.warn('Failed to check if node is replica: %s', err)
            __pillar__['__sqlserver_is_replica'] = None
    return __pillar__['__sqlserver_is_replica']


def endpoint_tcp_list(**kwargs):
    """
    function to list all sql server tcp endpoints and their status
    """
    sql = '''
    SELECT
        name,
        type_desc,
        state_desc,
        is_admin_endpoint,
        port,
        is_dynamic_port,
        ip_address
    FROM
        sys.tcp_endpoints
        '''
    try:
        conn = _get_connection(**kwargs)
        cur = conn.cursor()
        cur.execute(sql)
        result = [
            {
                'name': row[0],
                'type': row[1],
                'state': row[2],
                'is_admin': row[3],
                'port': row[4],
                'dynamic_port': row[5],
                'ip': row[6],
            }
            for row in cur.fetchall()
        ]

        return result
    except pyodbc.OperationalError as err:
        log.error('Could not enumerate endpoints: {0}'.format(err))
        log.exception(err)
        return False


def endpoint_tcp_enable(endpoint_name, **kwargs):
    """
    function to enable a disabled tcp endpoint in SQL Server.
    """
    sql = '''
    ALTER ENDPOINT {endpoint}
    STATE=STARTED
    '''.format(
        endpoint=_escape_obj_name(endpoint_name)
    )
    try:
        conn = _get_connection(**kwargs)
        cur = conn.cursor()
        cur.execute(sql)
        return True
    except pyodbc.OperationalError as err:
        log.error('Could not enable endpoint {0}: {1}'.format(endpoint_name, err))
        log.exception(err)
        return False


def endpoint_tcp_disable(endpoint_name, **kwargs):
    """
    function to disable tcp endpoint in SQL Server.
    """
    sql = '''
    ALTER ENDPOINT {endpoint}
    STATE=STOPPED
    '''.format(
        endpoint=_escape_obj_name(endpoint_name)
    )
    try:
        conn = _get_connection(**kwargs)
        cur = conn.cursor()
        cur.execute(sql)
        return True
    except pyodbc.OperationalError as err:
        log.error('Could not disable endpoint {0}: {1}'.format(endpoint_name, err))
        log.exception(err)
        return False


def endpoint_tcp_grant_connect(endpoint_name, login_name='public', **kwargs):
    """
    function to grant connect permission to specific login or role on a tcp endpoint in SQL Server.
    """
    sql = '''
    GRANT CONNECT ON ENDPOINT::{endpoint} TO {login}
    '''.format(
        endpoint=_escape_obj_name(endpoint_name), login=_escape_obj_name(login_name)
    )
    try:
        conn = _get_connection(**kwargs)
        cur = conn.cursor()
        cur.execute(sql)
        return True
    except pyodbc.OperationalError as err:
        log.error('Could not grant CONNECT permission on endpoint {0}: {1}'.format(endpoint_name, err))
        log.exception(err)
        return False


def endpoint_tcp_is_connect_granted(endpoint_name, login_name, **kwargs):
    """
    function to check if the specific login has CONNECT permission on a specific TCP endpoint in SQL Server
    """
    sql = '''
    DECLARE @login_name nvarchar(255) = ?, @ep_name nvarchar(255) = ?
    SELECT CASE WHEN EXISTS (
        SELECT 1 FROM
        sys.server_permissions sp
        JOIN sys.server_principals p on sp.grantee_principal_id = p.principal_id
        JOIN sys.server_principals p2 on p2.name = @login_name
        JOIN sys.tcp_endpoints ep on ep.endpoint_id = sp.major_id
        WHERE permission_name = 'CONNECT'
        and class_desc = 'ENDPOINT'
        and (p.name = 'public' or p.name = @login_name)
        and class = 105
        and ep.name = @ep_name
        and sp.state = 'G'
        )
        AND NOT EXISTS (
            SELECT 1 FROM
            sys.server_permissions sp
            JOIN sys.server_principals p on sp.grantee_principal_id = p.principal_id
            JOIN sys.tcp_endpoints ep on ep.endpoint_id = sp.major_id
            WHERE permission_name = 'CONNECT'
            and class_desc = 'ENDPOINT'
            and (p.name = 'public' or p.name = @login_name)
            and class = 105
            and ep.name = @ep_name
            and sp.state = 'D'
            )
        THEN 'True'
        ELSE NULL
        END as ConnectGranted
    '''
    vals = [login_name, endpoint_name]
    try:
        conn = _get_connection(**kwargs)
        cur = conn.cursor()
        cur.execute(sql, vals)
        result = [row[0] for row in cur.fetchall()]
        return result[0] == 'True'
    except pyodbc.OperationalError as err:
        log.error('Could not enumerate endpoints: {0}'.format(err))
        log.exception(err)
        return False


def endpoint_hadr_create(
    name='HADR_endpoint', ip='all', port=5022, cert_name=None, encryption='AES', replace=False, **kwargs
):
    """
    Function to create HADR endpoint
    """
    cert_sql = ''
    if cert_name:
        cert_sql = 'AUTHENTICATION = CERTIFICATE {cert}, ENCRYPTION = REQUIRED ALGORITHM {enc},'.format(
            cert=_escape_obj_name(cert_name), enc=_escape_obj_name(encryption)
        )
    op = 'CREATE'
    if replace == True:
        op = 'ALTER'
    sql = """
    {operation} ENDPOINT {name}
    STATE=STARTED
    AS TCP (
        LISTENER_PORT = {port},
        LISTENER_IP = {ip}
    )
    FOR DATABASE_MIRRORING (
        {auth}
        ROLE = ALL
        );

    """.format(
        operation=op, name=_escape_obj_name(name), port=str(port), ip=ip, auth=cert_sql
    )
    try:
        conn = _get_connection(**kwargs)
        cur = conn.cursor()
        cur.execute(sql)
        return True
    except pyodbc.OperationalError as err:
        log.error('Could not create endpoint {0}: {1}'.format(name, err))
        log.exception(err)
        return False


def dbs_list(type='all', state='all', **kwargs):
    """
    Function returns list of database from the SQL Server instance.
    Parameters:
        type = [all | system | user]
        applies filter, selecting all, system databases or user databases
        state = [all | online | offline | nonoperational]
        applies filter, selecting databases by their state
    """
    filter = ''
    if type == 'system':
        filter += 'AND database_id < 5'
    if type == 'user':
        filter += 'AND database_id > 4'
    if state == 'online':
        filter += 'AND state = 0'
    if state == 'offline':
        filter += 'AND state = 6'
    if state == 'nonoperational':
        filter += 'AND state between 1 and 10'

    sql = """
        SELECT name
        FROM sys.databases
        WHERE
             1 = 1
            {filter}
    """.format(
        filter=filter
    )

    conn = _get_connection(**kwargs)
    cur = conn.cursor()
    result = cur.execute(sql).fetchall()
    return [row[0] for row in result]


def backup_dbs_to_null(**kwargs):
    """
    backups to null all the user databases, that have no LastBackupDate
    """
    cmd = """
    [System.Reflection.Assembly]::LoadWithPartialName('Microsoft.SqlServer.SMO')|out-null

    $s = New-Object ('Microsoft.SqlServer.Management.Smo.Server') "localhost"
    $dbs = $s.Databases |where {$_.LastBackupDate -eq "01.01.0001 0:00:00" -and $_.ID -gt 4}

    $dbs|% -Process { Backup-SqlDatabase -serverinstance "localhost" -Database $_.Name -BackupFile 'null'}
    $dbs|Measure-Object|select count -expandproperty count"""

    ok, out, err = __salt__['mdb_windows.run_ps'](cmd)
    if ok:
        return True
    else:
        log.error(err)
        return False


def ag_create(
    ag_name='AG1',
    backup_preference='SECONDARY',
    failure_condition_level=3,
    healthcheck_timeout_ms=600000,
    db_failover='ON',
    av_mode='SYNCHRONOUS_COMMIT',
    fo_mode='AUTOMATIC',
    seeding_mode='AUTOMATIC',
    sec_allow_connections='ALL',
    backup_to_null=True,
    **kwargs
):
    """
    Function creates availability group with current instance as master
    """
    # Validate input

    invalid_args = {}

    if backup_preference not in PV['backup_preference']:
        invalid_args['backup_preference'] = 'backup_preference'
    if failure_condition_level not in PV['failure_condition_level']:
        invalid_args['failure_condition_level'] = failure_condition_level
    if db_failover not in PV['db_failover']:
        invalid_args['db_failover'] = db_failover
    if av_mode not in PV['av_mode']:
        invalid_args['av_mode'] = av_mode
    if fo_mode not in PV['fo_mode']:
        invalid_args['fo_mode'] = fo_mode
    if seeding_mode not in PV['seeding_mode']:
        invalid_args['seeding_mode'] = seeding_mode
    if sec_allow_connections not in PV['sec_allow_connections']:
        invalid_args['sec_allow_connections'] = sec_allow_connections

    if invalid_args:
        log.error(
            'Args validation failed:{0} '.format(','.join(arg + ': ' + str(val) for arg, val in invalid_args.items()))
        )
        return False

    servername = run_query('SELECT @@servername', **kwargs)
    if servername:
        servername = servername[0][0]

    fqdn = __salt__['mdb_windows.get_fqdn']()

    sql = """
    CREATE AVAILABILITY GROUP {ag_name}
    WITH (
    AUTOMATED_BACKUP_PREFERENCE = {backup_preference},
    FAILURE_CONDITION_LEVEL = {failure_condition_level},
    HEALTH_CHECK_TIMEOUT = {healthcheck_timeout_ms},
    DB_FAILOVER = {db_failover},
    DTC_SUPPORT = NONE
    )
    FOR REPLICA ON '{servername}'  WITH(
        ENDPOINT_URL = 'TCP://{fqdn}:5022',
        AVAILABILITY_MODE = {av_mode},
        FAILOVER_MODE = {fo_mode},
        SEEDING_MODE = {seeding_mode},
        SECONDARY_ROLE (
            ALLOW_CONNECTIONS = {sec_allow_connections}
        ),
        PRIMARY_ROLE (
            ALLOW_CONNECTIONS = ALL
        )
    );
    ALTER AVAILABILITY GROUP {ag_name} GRANT CREATE ANY DATABASE;
    """.format(
        ag_name=_escape_obj_name(ag_name),
        backup_preference=backup_preference,
        failure_condition_level=failure_condition_level,
        healthcheck_timeout_ms=healthcheck_timeout_ms,
        db_failover=db_failover,
        servername=servername,
        fqdn=fqdn,
        av_mode=av_mode,
        fo_mode=fo_mode,
        seeding_mode=seeding_mode,
        sec_allow_connections=sec_allow_connections,
    )
    conn = get_connection(**kwargs)

    try:
        conn = _get_connection(**kwargs)
        cur = conn.cursor()
        cur.execute(sql)
        return True
    except pyodbc.OperationalError as err:
        log.error('Could not create AG {0}: {1}'.format(ag_name, err))
        log.error(sql)
        log.exception(err)
        return False


def ag_list(**kwargs):
    """
    returns a list of availability groups, found on this replica and current role (PRIMARY or SECONDARY)
    """
    conn = get_connection(**kwargs)
    sql = """SELECT name,role_desc from sys.availability_groups ag
JOIN sys.dm_hadr_availability_replica_states rs ON ag.group_id = rs.group_id
    """
    ags = []
    try:
        ags = run_query(sql, **kwargs)
        return ags
    except pyodbc.OperationalError as err:
        log.error('Could get list of AGs: {1}'.format(err))
        log.exception(err)
        return False


def add_db_to_ag(db_name, ag_name=None, **kwargs):
    """
    Adds a database to an availability group. Can be run only from master.

    """
    # if no ag_name specified, then we just add it to a first available ag
    # whos primary is currently on this node.

    if not ag_name:
        ags = ag_list(**kwargs)
        for ag in ags:
            if ag[1] == 'PRIMARY':
                ag_name = ag[0]
                break
    if not ag_name:
        return False
    backup_dbs_to_null()
    sql = """ALTER AVAILABILITY GROUP {0}
    ADD DATABASE {1};
    select sys.fn_hadr_is_primary_replica({2});
    """.format(
        _escape_obj_name(ag_name), _escape_obj_name(db_name), _escape_string(db_name)
    )
    try:
        return run_query(sql, **kwargs)[0][0] == 1
    except pyodbc.OperationalError as err:
        log.error('Could not add database {0} to AG {1}: {2}'.format(db_name, ag_name, err))
        log.exception(err)
        return False


def db_ag_list(**kwargs):
    """
    Returns a list of all user databases with their AG name and current role
    """
    sql = """
SELECT dbs.name as [database_name], ISNULL(ag.name, 'None') as AG_name, ISNULL(ags.role_desc, 'None') as role
FROM sys.databases dbs
LEFT JOIN sys.dm_hadr_database_replica_states rs
		ON dbs.database_id = rs.database_id
LEFT JOIN sys.availability_groups ag
		ON ag.group_id = rs.group_id
LEFT JOIN sys.dm_hadr_availability_replica_states ags
		ON ags.group_id = ag.group_id
WHERE dbs.database_id > 4
"""
    try:
        return run_query(sql, **kwargs)
    except pyodbc.OperationalError as err:
        log.error('Could not list databases: {2}'.format(err))
        log.exception(err)
        return False


def run_sql_file(filename, **kwargs):
    """
    Function runs a specific .sql file in a local default instance of SQL Server
    using invoke-sqlcmd commandlet with trusted connection and -InputFile parameter
    """
    # cmd = "Invoke-sqlcmd -InputFile '{0}'".format(_escape_string(filename))

    proc = subprocess.Popen(
        ['sqlcmd', '-r0', '-i', filename], stdout=subprocess.PIPE, stderr=subprocess.PIPE, universal_newlines=True
    )
    out, err = proc.communicate()
    log.debug("SQLCMD RESULT: %s", out)
    log.debug("SQLCMD ERROR: %s", err)
    return proc.returncode == 0, out, err


def login_role_mod(login_name, roles_add, roles_drop, **kwargs):
    """
    Modifies login's role membership adding login in all roles listed in roles_add and dropping from all roles listed in roles_drop

    CLI Example:

    .. code-block:: bash

        salt minion mdb_sqlserver.login_role_mod LOGIN1 roles_add=['BULKADMIN'] roles_drop=['SYSADMIN']
    """

    try:
        server_roles = [v[0] for v in __salt__['mdb_sqlserver.server_role_list'](**kwargs)]

        cnxn = _get_connection(**kwargs)

        cur = cnxn.cursor()
        # if not a lists has been passed then we'll try to convert those to lists
        if type(roles_add) is not list and len(roles_add) > 0:
            roles_add = [roles_add]
        if type(roles_drop) is not list and len(roles_drop) > 0:
            roles_drop = [roles_drop]
        query = ''
        # build command to add user to all roles needed.
        if len(roles_add) > 0:
            for rl in roles_add:
                if rl in server_roles:
                    query += '''ALTER SERVER ROLE {0} ADD MEMBER {1};
'''.format(
                        _escape_obj_name(rl), _escape_obj_name(login_name)
                    )
                else:
                    log.error(
                        'Role {0} is not known and cannot be granted. Known roles are:'.format(
                            rl, ', '.join(server_roles)
                        )
                    )
                    return False
        if len(roles_drop) > 0:
            for rl in roles_drop:
                if rl in server_roles:
                    query += '''ALTER SERVER ROLE {0} DROP MEMBER {1};
'''.format(
                        _escape_obj_name(rl), _escape_obj_name(login_name)
                    )
                else:
                    log.error(
                        'Role {0} is not known and cannot be revoked. Known roles are:'.format(
                            rl, ', '.join(server_roles)
                        )
                    )
                    return False
        cur.execute(query)
        return True
    except pyodbc.OperationalError as err:
        log.error('Could not modify role membership: {0}'.format(err))
        log.exception(err)
        return False


def login_role_list(login, **kwargs):
    """
    Returns a list of server roles that a given login is member of
    """
    cur = _get_connection(**kwargs).cursor()
    query = '''
        SELECT roles.name
	FROM sys.server_principals princ
	JOIN sys.server_role_members members
		ON members.member_principal_id = princ.principal_id
	JOIN sys.server_principals roles
		ON roles.principal_id = members.role_principal_id
	WHERE princ.name = ?
                '''
    try:
        cur.execute(query, login)
        roles = [row[0] for row in cur.fetchall()]
        return roles
    except pyodbc.OperationalError as err:
        log.error('Could not get server roles list : {0}'.format(err))
        log.exception(err)
        return False
    except pyodbc.ProgrammingError as err:
        log.error('Could not get login roles list : {0}'.format(err))
        log.exception(err)
        return False


def server_role_list(**kwargs):
    """
    Returns a list of server-level roles.
    """
    query = '''
        SELECT name
        FROM sys.server_principals
        WHERE type='R' AND name <> 'public'
    '''
    try:
        return run_query(query, **kwargs)
    except pyodbc.OperationalError as err:
        log.error('Could not get server roles list : {0}'.format(err))
        log.exception(err)
        raise
    except pyodbc.ProgrammingError as err:
        log.error('Could not get server roles list : {0}'.format(err))
        log.exception(err)
        raise


def server_role_permissions_list(role, **kwargs):
    """
    Returns the list of permissions that are granted to a certain server role
    """
    cur = _get_connection(**kwargs).cursor()
    query = '''
        SELECT permission_name
         FROM
            sys.server_permissions perm
            JOIN sys.server_principals prin
                ON perm.grantee_principal_id = prin.principal_id
         WHERE prin.name = ?
            '''
    try:
        cur.execute(query, role)
        privs = [row[0] for row in cur.fetchall()]
        return privs
    except pyodbc.OperationalError as err:
        log.error('Could not get permissions list : {0}'.format(err))
        log.exception(err)
        raise
    except pyodbc.ProgrammingError as err:
        log.error('Could not get permissions list : {0}'.format(err))
        log.exception(err)
        raise


def server_role_permissions_mod(role, permissions_add, permissions_drop, **kwargs):
    """
    Grants or revokes permissions to a server role.
        role - name of the role to alter
        permissions_add - list of permissions to add to a role
        permissions_drop - list of permissions to revoke from role
    """
    try:
        cur = _get_connection(**kwargs).cursor()
        # if not a lists has been passed then we'll try to convert those to lists
        if type(permissions_add) is not list and len(permissions_add) > 0:
            permissions_add = [permissions_add]
        if type(permissions_drop) is not list and len(permissions_drop) > 0:
            permissions_drop = [permissions_drop]
        query = ''
        # build command to add requested permissions
        if len(permissions_add) > 0:
            for perm in permissions_add:
                if perm in [v[0] for v in SERVER_PRIVS]:
                    query += '''GRANT {0} TO {1};
'''.format(
                        perm, _escape_obj_name(role)
                    )  # escaping is not possible for permission name as some permissions consist of several key words (VIEW SERVER STATE)
                else:
                    log.error('Permission {0} is not known and cannot be granted'.format(perm))
                    return False
        if len(permissions_drop) > 0:
            for perm in permissions_drop:
                if perm in [v[0] for v in SERVER_PRIVS]:
                    query += '''REVOKE {0} TO {1} AS [sa];
'''.format(
                        perm, _escape_obj_name(role)
                    )
                else:
                    log.error('Permission {0} is not known and cannot be revoked'.format(perm))
                    return False
        cur.execute(query)
    except pyodbc.OperationalError as err:
        log.error('Could not modify permissions of role {0}: {1}'.format(role, err))
        log.exception(err)
        return False
    except pyodbc.ProgrammingError as err:
        log.error('Could not modify permissions of role {0}: {1}'.format(role, err))
        log.exception(err)
        return False
    return True


def server_role_create(role, **kwargs):
    """
    Creates the server role. No or members permissions assigned
    """
    cur = _get_connection(**kwargs).cursor()
    query = "CREATE SERVER ROLE {0}".format(_escape_obj_name(role))
    try:
        cur.execute(query)
    except pyodbc.OperationalError as err:
        log.error('Could not create server role {0}: {1}'.format(role, err))
        log.exception(err)
        return False
    except pyodbc.ProgrammingError as err:
        log.error('Could not create server role {0}: {1}'.format(role, err))
        log.exception(err)
        return False
    return True


def server_role_delete(role, **kwargs):
    """
    Creates the server role. No or members permissions assigned
    """
    cur = _get_connection(**kwargs).cursor()
    query = "DROP SERVER ROLE {0}".format(_escape_obj_name(role))
    try:
        cur.execute(query)
    except pyodbc.OperationalError as err:
        log.error('Could not drop server role {0}: {1}'.format(role, err))
        log.exception(err)
        return False
    except pyodbc.ProgrammingError as err:
        log.error('Could not drop server role {0}: {1}'.format(role, err))
        log.exception(err)
        return False
    return True


def get_folder_by_version(version_string):
    """
    Helper function that returns a version specific part of SQL Server root directory path.
    """
    versions = {
        "2016sp2std": "MSSQL13.MSSQLSERVER",
        "2016sp2ent": "MSSQL13.MSSQLSERVER",
        "2016sp2dev": "MSSQL13.MSSQLSERVER",
        "2017dev": "MSSQL14.MSSQLSERVER",
        "2017std": "MSSQL14.MSSQLSERVER",
        "2017ent": "MSSQL14.MSSQLSERVER",
        "2019dev": "MSSQL15.MSSQLSERVER",
        "2019std": "MSSQL15.MSSQLSERVER",
        "2019ent": "MSSQL15.MSSQLSERVER",
    }
    return versions[version_string]


def get_ag_replicas_properties(**kwargs):
    """
    Returns a dict of properties of all replicas, like this:
    replica_host_name:
        ----------
        AG_name:
            ----------
            availability_mode:
                SYNCHRONOUS_COMMIT
            failover_mode:
                AUTOMATIC
            primary_allow_connections:
                ALL
            secondary_allow_connections:
                NO
            seeding_mode:
                MANUAL
    """
    conn = get_connection(**kwargs)
    cur = conn.cursor()
    sql = """SELECT
                ag.name as ag_name,
                ar.replica_server_name,
                    availability_mode_desc as availability_mode,
                    failover_mode_desc as failover_mode,
                    primary_role_allow_connections_desc as primary_allow_connections,
                    secondary_role_allow_connections_desc as secondary_allow_connections,
                    seeding_mode_desc as seeding_mode
                FROM sys.availability_replicas ar
                JOIN sys.availability_groups ag
                    ON ag.group_id = ar.group_id
            """
    result = cur.execute(sql).fetchall()
    actual_state = {}

    for r in result:
        if not actual_state.get(r[1], ''):
            actual_state[r[1]] = {}
        actual_state[r[1]][r[0]] = {
            'availability_mode': r[2],
            'failover_mode': r[3],
            'primary_allow_connections': r[4],
            'secondary_allow_connections': r[5],
            'seeding_mode': r[6],
        }
    return actual_state


def make_db_ag_map(dbs, db_filter, edition):
    if db_filter:
        dbs = [db for db in dbs if db == db_filter]
    else:
        dbs = list(dbs)
    if edition == 'standard':
        db_ag = {d: d for d in dbs}
    else:
        db_ag = {d: 'AG1' for d in dbs}
    db_list = list(db_ag.keys())
    ag_list = list(set(db_ag.values()))
    return db_ag, db_list, ag_list


def check_external_storage(pillar, operation='backup-import', saltenv='prod', **kwargs):
    assert isinstance(pillar, dict)
    __pillar__.update(pillar)
    res = __salt__['file.get_managed'](
        name='123.yaml',
        template='jinja',
        source='salt://components/sqlserver/conf/external-wal-g.yaml',
        source_hash=None,
        source_hash_name=None,
        user=None,
        group=None,
        mode=None,
        attrs=None,
        saltenv=saltenv,
        context={
            'operation': operation,
        },
        defaults={},
        skip_verify=True,
    )
    tmpconf = res[0] + '.yaml'
    os.rename(res[0], tmpconf)
    CMD = [
        'C:\Program Files\wal-g-sqlserver\wal-g-sqlserver.exe',
        '--config', tmpconf,
        'st', 'check'
    ]
    if operation == 'backup-import':
        CMD += ['read'] + __salt__['pillar.get']('backup-import:s3:files', [])
    else:
        CMD += ['write']
    try:
        subprocess.check_output(CMD, universal_newlines=True, stderr=subprocess.STDOUT)
        return {'is_storage_ok': True}
    except subprocess.CalledProcessError as err:
        info = err.output
        for line in err.output.split('\n'):
            if line.startswith('ERROR'):
                info = line
                break
        return {'is_storage_ok': False, 'info': info}
    finally:
        try:
            os.remove(tmpconf)
        except:
            pass
