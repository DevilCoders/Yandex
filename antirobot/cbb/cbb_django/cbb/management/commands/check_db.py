from antirobot.cbb.cbb_django.cbb.library import db
from django.core.management.base import BaseCommand, CommandError


class Command(BaseCommand):
    help = 'Get databases status'
    args = '<db>'

    def handle(self, *args, **options):
        if len(args) != 1 or args[0] not in {'main', 'history'}:
            raise CommandError("Invalid argument. Must be 'main' or 'history'.")

        if args[0] == 'history':
            statuses = db.history.db_statuses(refresh=True)
        else:
            statuses = db.main.db_statuses(refresh=True)

        if not statuses['master']:
            self.stdout.write('2;Master failed')
        elif False in statuses.values():
            slaves = [str(slave) for slave in statuses.keys() if not statuses[slave]]
            self.stdout.write("1;Slave(s) {0} failed".format(''.join(slaves)))
        else:
            self.stdout.write('0;OK')
