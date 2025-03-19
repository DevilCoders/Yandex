#!/usr/bin/env python3

import argparse
import configparser
import logging
import logging.handlers
from datetime import timedelta
import psycopg2

DEFAULT_OLD_INTERVAL = timedelta(hours=1)
STATUSES = [
    'PLANNED',
    'CREATING',
    'DONE',
    'OBSOLETE',
    'DELETING',
    'DELETED',
    'CREATE-ERROR',
    'DELETE-ERROR',
]

log = logging.getLogger(__name__)


def updated_at_before(cur, args):
    cur.execute(
        """
        SELECT b.backup_id
        FROM dbaas.backups b
        JOIN dbaas.clusters c USING(cid)
        WHERE b.updated_at < (now() - interval %s)
          AND b.status in %s
          AND c.type = %s
          AND dbaas.visible_cluster_status(c.status)
        ORDER BY b.updated_at;
        """,
        [args.interval, args.statuses, args.cluster_type],
    )
    return cur.fetchall()


def delayed_until_before(cur, args):
    cur.execute(
        """
       SELECT b.backup_id
       FROM   dbaas.backups b
       JOIN   dbaas.clusters c USING(cid)
       WHERE  b.delayed_until < (now() - interval %s)
         AND b.status in %s
         AND c.type = %s
         AND dbaas.visible_cluster_status(c.status)
       ORDER BY b.delayed_until;
       """,
        [args.interval, args.statuses, args.cluster_type],
    )
    return cur.fetchall()


def finished_at_after(cur, args):
    cur.execute(
        """
       SELECT b.backup_id
       FROM   dbaas.backups b
       JOIN   dbaas.clusters c USING(cid)
       WHERE  b.finished_at > (now() - interval %s)
         AND b.status in %s
         AND c.type = %s
         AND dbaas.visible_cluster_status(c.status)
       ORDER BY b.finished_at;
       """,
        [args.interval, args.statuses, args.cluster_type],
    )
    return cur.fetchall()


def interval_arg(seconds):
    return timedelta(seconds=int(seconds))


def statuses_arg(statuses_str):
    statuses = []
    for status in statuses_str.split(','):
        if status not in STATUSES:
            raise ValueError('Unknown backup status: {0}'.format(status))
        statuses.append(status)
    return tuple(statuses)


def main():
    logging.basicConfig(format='%(asctime)s %(funcName)s [%(levelname)s]: %(message)s', level=logging.DEBUG)

    parser = argparse.ArgumentParser(
        """
        Backup service queue monitoring.
        """
    )
    parser.add_argument(
        "-c",
        "--config",
        help="path to config",
        default="/etc/yandex/mdb-backup/monitoring.cfg",
    )
    base_cmd_parser = argparse.ArgumentParser(add_help=False)
    base_cmd_parser.add_argument(
        '--crit', type=int, required=False, default=1, metavar='<integer>', help='Critical threshold'
    )
    base_cmd_parser.add_argument(
        '--warn', type=int, required=False, default=1, metavar='<integer>', help='Warning threshold'
    )
    base_cmd_parser.add_argument('--interval', type=interval_arg, required=True, help='Interval in seconds')
    base_cmd_parser.add_argument('--cluster-type', type=str, required=True, help='Run check for cluster type')
    base_cmd_parser.add_argument(
        '--statuses', type=statuses_arg, required=True, help='Backup statuses to select, comma-separated'
    )
    base_cmd_parser.add_argument(
        '--report-format',
        type=str,
        required=True,
        help='Output string (str.format): "Description (over {interval}): {count}. Example: {example}"',
    )

    subparsers = parser.add_subparsers(title='Checks')

    updated_at_before_parser = subparsers.add_parser(
        'updated_at_before',
        parents=[base_cmd_parser],
        help='Number of tasks with given statuses and updated_at older than specified interval',
    )
    updated_at_before_parser.set_defaults(func=updated_at_before)

    delayed_until_before_parser = subparsers.add_parser(
        'delayed_until_before',
        parents=[base_cmd_parser],
        help='Number of tasks with given statuses and delayed_until older than specified interval',
    )
    delayed_until_before_parser.set_defaults(func=delayed_until_before)

    finished_at_after_parser = subparsers.add_parser(
        'finished_at_after',
        parents=[base_cmd_parser],
        help='Number of tasks with given statuses and finished_at newer than specified interval',
    )
    finished_at_after_parser.set_defaults(func=finished_at_after)

    args = parser.parse_args()

    try:
        cfg = configparser.ConfigParser()
        cfg.read([args.config])

        with psycopg2.connect(cfg.get("metadb", "dsn")) as conn:
            with conn.cursor() as cur:
                rows = args.func(cur, args)
                count = len(rows)
                if count >= args.crit:
                    code = 2
                elif count >= args.warn:
                    code = 1
                else:
                    print('0;OK')
                    return

                print(
                    '{0};{1}'.format(
                        code, args.report_format.format(interval=args.interval, count=count, example=rows[0][0])
                    )
                )

    except AttributeError as exc:
        print(exc)
        parser.print_help()
        parser.exit()


if __name__ == "__main__":
    main()
