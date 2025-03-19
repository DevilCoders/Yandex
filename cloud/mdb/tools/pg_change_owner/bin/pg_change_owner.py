import argparse
import logging

from .logs import conf_logging
from cloud.mdb.tools.pg_change_owner.internal.logic import change_owner_in_all_objects, get_connection, execute_extra

logger: logging.Logger


def main():
    """
    RUN: --conn "" --database deploydb --user deploydb_admin --schema deploy --schema code
    """
    global logger
    conf_logging()
    logger = logging.getLogger(__name__)
    parser = argparse.ArgumentParser()
    parser.add_argument(
        "-c",
        "--conn",
        help="database connection string",
    )
    parser.add_argument(
        "-u",
        "--user",
        dest="user",
        help="user to own objects",
    )
    parser.add_argument(
        "-d",
        "--database",
        dest="database",
        help="database to own objects",
    )
    parser.add_argument(
        "-s",
        "--schema",
        dest="schemas",
        action='append',
        help="schemas to change",
    )
    parser.add_argument(
        "-e",
        "--extra",
        dest="extra_queries",
        action='append',
        help="extra AD-HOC queries to execute",
    )
    args = parser.parse_args()

    conn = get_connection(args.conn)

    change_owner_in_all_objects(
        connection=conn,
        db_name=args.database,
        schemas=args.schemas,
        owner=args.user,
    )

    extra_queries = args.extra_queries or []
    execute_extra(
        connection=conn,
        extra_queries=extra_queries,
    )


if __name__ == "__main__":
    main()
