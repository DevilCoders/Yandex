import datetime
from antirobot.cbb.cbb_django.cbb.library import db
from antirobot.cbb.cbb_django.cbb.models import LIMBO_VERSIONS
from django.core.management.base import NoArgsCommand
from sqlalchemy import func


class Command(NoArgsCommand):
    help = "Check how old is history limbos oldest entry"

    @db.main.use_slave()
    def handle_noargs(self, **options):
        timestamps = []
        for LimboClass in LIMBO_VERSIONS.values():
            last_time = db.main.session.query(func.min(LimboClass.unblocked_at)).first()[0]
            if last_time:
                timestamps.append(last_time)

        if not timestamps:
            self.stdout.write('0;OK')
            return

        time_d = (datetime.datetime.now() - min(timestamps))
        time_d -= datetime.timedelta(microseconds=time_d.microseconds)

        if time_d.total_seconds() > 3600 * 24:
            self.stdout.write(
                "2;History limbos not flushed for {0}\n".format(
                    time_d))
        elif time_d.total_seconds() > 3600:
            self.stdout.write(
                "1;History limbos not flushed for {0}\n".format(
                    time_d))
        else:
            self.stdout.write('0;OK')
