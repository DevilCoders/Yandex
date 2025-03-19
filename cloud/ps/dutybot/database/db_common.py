import logging

import psycopg2

from bot_utils.config import Config
from bot_utils.errors import Error


def isolate_strings(data):
    if not isinstance(data, dict):
        logging.error(f"Invalid data {data}")
        raise Error(f"Expected dict, but {type(data)} provided")

    return {key: f"'{value}'" for key, value in data.items()}


def create_connection():
    """
    Creates psycopg2 connection from config
    and returns it
    """
    connection = psycopg2.connect(
        user=Config.db_user,
        password=Config.db_pass,
        host=",".join(Config.db_host),
        port=Config.db_port,
        database=Config.db_name,
        target_session_attrs="read-write",
    )
    return connection
