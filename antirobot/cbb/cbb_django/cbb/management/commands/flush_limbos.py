import logging
from antirobot.cbb.cbb_django.cbb.library import db, lock
import antirobot.cbb.cbb_django.cbb.models as models
from django.core.management.base import BaseCommand, CommandError


logger = logging.getLogger("cbb.cron.history")


@db.main.use_master()
@db.history.use_master()
def flush_limbos(batch_size=None):
    """
    Removes blocks from limbos and adds its to history.
    """
    result = {}
    version_to_str = {4: "IPV4", 6: "IVP6", "txt": "Txt", "re": "Re"}

    for version in models.LIMBO_VERSIONS.keys():
        LimboClass = models.LIMBO_VERSIONS[version]
        HistoryClass = models.HISTORY_VERSIONS[version]
        count = LimboClass.query.count()
        if not batch_size:
            batch_size = count

        processed_count = 0
        while processed_count < count:
            if count - processed_count < batch_size:
                batch_size = count - processed_count
            limbo_entries = LimboClass.query.limit(batch_size).all()
            if not limbo_entries:
                break
            history_entries = [
                HistoryClass.from_limbo(limbo) for limbo in limbo_entries]
            db.history.session.add_all(history_entries)
            db.history.session.commit()

            for e in limbo_entries:
                db.main.session.delete(e)
            db.main.session.commit()
            processed_count += batch_size
        result[version_to_str[version]] = processed_count
    return result


class Command(lock.LockedCommandMixin, BaseCommand):
    help = "Delete blocks from limbo history table and add to history DB"

    def add_arguments(self, parser):
        parser.add_argument(
            "-b",
            "--batch_size",
            action="store",
            dest="batch_size",
            type=int,
            default=1000,
            help="Commit to db in batches"
        )

    def _handle(self, *args, **options):
        batch_size = options["batch_size"]

        if not isinstance(batch_size, int) or batch_size <= 0:
            raise CommandError("Batch size must be positive integer.")

        start_message = "Start processing limbos"
        logger.info(start_message)
        self.stdout.write(start_message)

        result_str = ", ".join(["{0}={1}".format(k, v) for k, v in flush_limbos(batch_size).items()])

        result_message = "Flushed limbos count: {0}".format(result_str)
        logger.info(result_message)
        self.stdout.write(result_message)
