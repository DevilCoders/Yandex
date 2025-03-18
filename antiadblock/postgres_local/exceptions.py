class PostgresqlException(Exception):
    pass


class MigrationException(PostgresqlException):
    pass
