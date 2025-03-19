import argparse
import logging
import os
from pathlib import Path

from cloud.mdb.internal.python.pg_create_users.internal.base import (
    Secrets,
    get_connection,
    _get_current_region,
    role_exists,
    create_role,
    conf_logging,
    alter_role,
    role_has_options,
)


def main():
    conf_logging()
    logger = logging.getLogger(__name__)
    parser = argparse.ArgumentParser()
    parser.add_argument(
        "-g",
        "--grants",
        help="path to grants dir",
    )
    parser.add_argument(
        "-a",
        "--application",
        help="application name",
    )
    parser.add_argument(
        "-c",
        "--conn",
        help="database connection string",
    )
    parser.add_argument(
        "-u",
        "--user",
        dest="users",
        action="append",
        help="user to create, can be used multiple times",
    )
    parser.add_argument('--connection-limit', type=int, default=100, help='connection limit for all user')
    parser.add_argument('--lock-timeout', default='10s', help='lock_timeout for all user')
    args = parser.parse_args()

    app_name = args.application
    conn = get_connection(args.conn)
    secrets = Secrets(_get_current_region(), logger=logger)
    cp_name = os.environ.get('CONTROLPLANE_NAME')
    aws_rds_iam_auth = os.environ.get('AWS_RDS_IAM_AUTH', 'False').lower() in ('true', '1')
    users = args.users
    if users is None:
        users = []
        for grant_file in Path(args.grants).glob("*.sql"):
            user_name = grant_file.name
            users.append(user_name[: user_name.rfind(".")])

    for user_name in users:
        password = None
        # No need to fetch secrets from AWS SSM when AWS RDS IAM Auth is set to True
        if not aws_rds_iam_auth:
            password = secrets.get_role_password(f'{cp_name}/{app_name}/{user_name}')

        login = True

        # When AWS RDS IAM Auth is set to True, it is required to create an user with Login=True
        if password is None and not aws_rds_iam_auth:
            logger.info("password is None and aws rds iam auth is set to False, setting login to False")
            login = False

        if role_exists(conn, user_name):
            logger.info("role %s already exists, changing", user_name)
            alter_role(conn, name=user_name, password=password, connection_limit=args.connection_limit, aws_rds_iam_auth=aws_rds_iam_auth)
        else:
            logger.info('creating role %s', user_name)
            create_role(conn, name=user_name, password=password, login=login, connection_limit=args.connection_limit, aws_rds_iam_auth=aws_rds_iam_auth)
        role_has_options(logger, conn, user_name=user_name, options={'lock_timeout': args.lock_timeout})
        conn.commit()


if __name__ == "__main__":
    main()
