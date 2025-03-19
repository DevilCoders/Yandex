"""
Constats for mysql backup
"""
import re
import socket

HOSTNAME = socket.gethostname()

REPLICA_API = "http://c.yandex-team.ru/api/generator/mysql_replicas"
BINLOG_RE = re.compile(r"(?P<name>.+)\.(?P<len>0*(?P<num>\d+))$")
LOCK_FLAG = "/var/tmp/mysql-backup-lock-acquired"
