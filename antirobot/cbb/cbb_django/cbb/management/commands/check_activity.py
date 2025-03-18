import datetime
import yenv
from antirobot.cbb.cbb_django.cbb.library import db
from antirobot.cbb.cbb_django.cbb.models import Group
from django.core.management.base import NoArgsCommand
from sqlalchemy import func


class Command(NoArgsCommand):
    help = "Get recording activity status"

    @db.main.use_slave()
    def handle_noargs(self, **options):
        if yenv.type == "testing":
            self.stdout.write("0;OK")  # cause testing has no activity
            return

        last_time = db.main.session.query(func.max(Group.updated)).first()[0]
        time_d = datetime.datetime.now() - last_time
        time_d -= datetime.timedelta(microseconds=time_d.microseconds)

        if time_d.total_seconds() > 3600:
            self.stdout.write("2;Last activity was more than an hour ago: {0} passed".format(time_d))
        elif time_d.total_seconds() > 1800:
            self.stdout.write("1;Last activity was more than half an hour ago: {0} passed".format(time_d))
        else:
            self.stdout.write("0;OK")
