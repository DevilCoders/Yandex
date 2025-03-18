# coding: utf-8

import os
import signal
import subprocess

from django.core.management.base import BaseCommand
from django.db import DEFAULT_DB_ALIAS, connections


class Command(BaseCommand):
    """
    The command is a simpler alternative to `dbshell`.
    Unlike `dbshell`, it uses all connection parameters, including OPTIONS.
    """

    help = (
        'Runs the command-line client for specified database, or the '
        'default database if none is provided. '
        'Unlike dbshell, uses all connection parameters from settings.'
    )

    def add_arguments(self, parser):
        parser.add_argument(
            '--database',
            action='store',
            dest='database',
            default=DEFAULT_DB_ALIAS,
            help=(
                'Nominates a database onto which to open a shell. '
                'Defaults to the "default" database.'
            ),
        )
        parser.add_argument(
            'cmd',
            nargs='?',
            help='Optional command string to execute via `psql -c`',
        )

    def handle(self, *args, **kwargs):

        conn = connections[kwargs['database']]

        params = conn.get_connection_params()
        params['dbname'] = params.pop('database', '')

        env = os.environ.copy()
        # Pass password in an environment variable lest it appear in `ps` output
        env['PGPASSWORD'] = params.pop('password', '')

        conn_string = ' '.join("%s='%s'" % kv for kv in params.items())

        sigint_handler = signal.getsignal(signal.SIGINT)

        args = ['psql', conn_string]

        cmd = kwargs.get('cmd')
        if cmd:
            args.extend(('-c', cmd))

        try:
            signal.signal(signal.SIGINT, signal.SIG_IGN)
            subprocess.check_call(args, env=env)

        finally:
            signal.signal(signal.SIGINT, sigint_handler)
