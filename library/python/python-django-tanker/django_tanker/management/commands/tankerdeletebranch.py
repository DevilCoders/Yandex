# coding: utf-8
from django_tanker.management.commands import TankerCommand


class Command(TankerCommand):
    help = u'Deletes a branch if it exists'

    def handle(self, *args, **options):
        self.options = self.configure_options(**options)

        if self.tanker.branch_description(options['branch']):
            self.tanker.delete_branch(options['branch'])
