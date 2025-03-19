import json
import logging
from logging.config import dictConfig
from typing import Optional

import boto3
import psycopg2
import requests
from botocore.exceptions import ClientError


def role_exists(conn, name):
    cur = conn.cursor()
    cur.execute("SELECT 1 FROM pg_roles WHERE rolname=%s", (name,))
    return cur.fetchone() is not None


def alter_role(conn, name, password, connection_limit, aws_rds_iam_auth):
    cur = conn.cursor()
    if password is None:
        # When AWS RDS IAM Auth is enabled, the only thing we might wish
        # to alter is the connection_limit parameter
        if aws_rds_iam_auth:
            cur.execute(f'ALTER ROLE {name} LOGIN CONNECTION LIMIT {connection_limit}')
        else:
            cur.execute(f"ALTER ROLE {name} WITH PASSWORD NULL NOLOGIN CONNECTION LIMIT {connection_limit}")
    else:
        cur.execute(
            "ALTER ROLE {name} WITH PASSWORD '{password}' LOGIN CONNECTION LIMIT {connection_limit}".format(
                password=cur.mogrify(password).decode('utf-8'),
                name=name,
                connection_limit=connection_limit,
            )
        )


def create_role(conn, name, password, login, connection_limit, aws_rds_iam_auth):
    cur = conn.cursor()
    options = ["LOGIN" if login else "NOLOGIN"]
    if connection_limit is not None:
        options.append(f"CONNECTION LIMIT {connection_limit}")
    if password is not None:
        options.append("PASSWORD '%s'" % cur.mogrify(password).decode('utf-8'))
    cur.execute(f'CREATE ROLE {name} WITH {" ".join(options)}')
    if aws_rds_iam_auth:
        cur.execute(f'GRANT rds_iam TO {name}')


def parse_role_config(role_config: Optional[list[str]]) -> dict[str, str]:
    if not role_config:
        return {}
    # each option is pair
    # TimeZone=utc,log_statement=all
    ret = {}
    for conf_pair in role_config:
        conf_name, _, conf_value = conf_pair.partition("=")
        if conf_name in ret:
            raise RuntimeError(f'Duplicate {conf_name} definition in {role_config}')
        ret[conf_name] = conf_value
    return ret


def role_has_options(logger, conn, user_name: str, options: dict[str, str]) -> None:
    """
    Verify that role has options
    Expect that given role already exists.
    """
    cur = conn.cursor()

    def set_option(name: str, value: str) -> None:
        cur.execute(
            "ALTER ROLE {user_name} SET {name} TO '{value}'".format(
                user_name=user_name,
                name=name,
                value=cur.mogrify(value).decode('utf-8'),
            )
        )

    def reset_option(name: str) -> None:
        cur.execute(f"ALTER ROLE {user_name} RESET {name}")

    cur.execute("SELECT rolconfig FROM pg_roles WHERE rolname = %s", (user_name,))
    role_config = parse_role_config(cur.fetchone()[0])

    for current_name, current_value in role_config.items():
        if current_name not in options:
            logger.info(
                "resetting %s %s=%s, cause it's not preset in options: %r",
                user_name,
                current_name,
                current_value,
                options,
            )
            reset_option(current_name)
            continue

        if current_value != options[current_name]:
            logger.info("changing %s %s from %s to %s", user_name, current_name, current_value, options[current_name])
            set_option(current_name, options[current_name])

    for new_name in set(options) - set(role_config):
        new_value = options[new_name]
        logger.info("setting %s %s to %s", user_name, new_name, new_value)
        set_option(new_name, new_value)


class Secrets:
    def __init__(self, region_name, logger=None):
        session = boto3.session.Session()
        self._client = session.client(
            service_name="secretsmanager",
            region_name=region_name,
        )
        self.logger = logger or logging.getLogger(__name__)

    def get_role_password(self, name):
        try:
            get_secret_value_response = self._client.get_secret_value(SecretId=name)
        except ClientError as e:
            error_tpl = "failed to get secret value for SecretId %s: %s, %s"
            if e.response["Error"]["Code"] == "ResourceNotFoundException":
                self.logger.warning(
                    error_tpl,
                    name,
                    e.response["Error"]["Code"],
                    e.response["Error"]["Message"],
                )
                return None
            if e.response["Error"]["Code"] == "InvalidRequestException":
                if "marked for deletion" in e.response["Error"]["Message"]:
                    self.logger.warning(
                        error_tpl,
                        name,
                        e.response["Error"]["Code"],
                        e.response["Error"]["Message"],
                    )
                    return None
            raise
        if "SecretString" not in get_secret_value_response:
            raise RuntimeError("SecretString not found in get_secret_value_response. SecretBinary not supported.")

        full_secret = get_secret_value_response["SecretString"]
        return json.loads(full_secret)["password"]


def _get_current_region():
    """
    I don't find a way to get a current region with boto3.
    So read it from metadata
    """
    resp = requests.get("http://169.254.169.254/latest/dynamic/instance-identity/document")
    resp.raise_for_status()
    return resp.json()["region"]


def get_connection(connection_string: str):
    return psycopg2.connect(connection_string)


def conf_logging():
    dictConfig(
        {
            'version': 1,
            'disable_existing_loggers': True,
            'formatters': {
                'default': {
                    'class': 'logging.Formatter',
                    'datefmt': '%Y-%m-%d %H:%M:%S',
                    'format': '%(asctime)s %(name)-15s %(levelname)-10s %(message)s',
                },
            },
            'handlers': {
                'streamhandler': {
                    'level': 'DEBUG',
                    'class': 'logging.StreamHandler',
                    'formatter': 'default',
                    'stream': 'ext://sys.stdout',
                },
                'null': {
                    'level': 'DEBUG',
                    'class': 'logging.NullHandler',
                },
            },
            'loggers': {
                '': {
                    'handlers': [
                        'streamhandler',
                    ],
                    'level': 'DEBUG',
                },
            },
            'root': {'level': 'DEBUG', 'handlers': ['streamhandler']},
        }
    )
