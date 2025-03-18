from antirobot.cbb.cbb_django.cbb.library import db
from django.core.management.base import NoArgsCommand
from antirobot.cbb.cbb_django.cbb.models import HISTORY_VERSIONS


class Command(NoArgsCommand):

    @db.history.use_master()
    def handle_noargs(self, **options):
        for version in (4, 6):
            HistoryClass = HISTORY_VERSIONS[version]

            candidates = HistoryClass.query.filter(HistoryClass.rng_start != HistoryClass.rng_end)
            self.stdout.write("v" + str(version) + " candidates:" + str(candidates.count()))

            affected_count = 0
            for candidate in candidates:
                xtra_large = (int(candidate.get_rng_end()) - int(candidate.get_rng_start())) > 255
                if xtra_large and not candidate.xtra_large:
                    candidate.xtra_large = True
                    affected_count += 1
            db.history.session.commit()
            self.stdout.write("v" + str(version) + " affected: " + str(affected_count))
