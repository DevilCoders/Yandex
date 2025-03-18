# coding: utf-8
from django.conf import settings

from django_tanker.management.commands import TankerCommand


class Command(TankerCommand):
    help = u'Creates a branch if it does not exist, returns its ref'

    def add_arguments(self, parser):
        super(Command, self).add_arguments(parser)
        parser.add_argument('--from', action='store', default='master',
                            help=u'Branch name')

    def handle(self, *args, **options):
        self.options = self.configure_options(**options)

        if options.get('from') is None:
            # если не указано, то создаем ветку от TANKER_REVISION
            options['from'] = getattr(settings, 'TANKER_REVISION', 'master')

        branch_description = self.tanker.branch_description(options['branch'])
        if not branch_description:
            self.tanker.create_branch(
                branch_name=options['branch'],
                from_branch=options['from']
            )
            branch_description = self.tanker.branch_description(options['branch'])
        print('Latest ref for branch "{branch}" is "{ref}"'.format(
            branch=options['branch'],
            ref=branch_description['ref'],
        ))
