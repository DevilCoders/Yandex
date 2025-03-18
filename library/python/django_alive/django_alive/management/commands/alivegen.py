# coding: utf-8
from __future__ import unicode_literals, absolute_import

import sys

from django.core.management.base import CommandError
from django.template.loader import render_to_string

from .base import AliveBaseCommand, Argument
from ...loading import get_group_checks


class Command(AliveBaseCommand):
    help = 'Do checks'

    arguments = AliveBaseCommand.arguments + [
        Argument('-u', '--user', 'store', 'www-data', None, 'User for cron execution'),
        Argument('-p', '--prog', 'store', None, None, 'Program name'),
        Argument('-r', '--prefix', 'store', None, None, 'Monrun check name prefix'),
        Argument('-a', '--aggregate', 'store_true', False, None, 'Aggregate checks with reduce'),
    ]

    def add_arguments(self, parser):
        super(Command, self).add_arguments(parser)
        parser.add_argument(
            'tpl',
            nargs='?',
        )

    def handle(self, tpl, **options):
        context = {'prog': options['prog'] or sys.argv[0],
                   'user': options['user'],
                   'groups': options['group'],
                   'checks': options['check']}

        context['prefix'] = options['prefix'] or context['prog']

        if tpl == 'cron':
            self.stdout.write(render_to_string('alive/gen/cron.tpl', context))
        elif tpl == 'monrun':
            if not context['groups']:
                raise CommandError('Group must be specified')

            if len(context['groups']) > 1:
                raise CommandError('Only one group or check can be specified')

            context['group'] = context.pop('groups')[0]

            if not context['checks'] and not options['aggregate']:
                context['checks'] = get_group_checks(context['group'])

            self.stdout.write(render_to_string('alive/gen/monrun.tpl', context))
