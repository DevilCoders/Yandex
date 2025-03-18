import logging
from datetime import datetime
from antirobot.cbb.cbb_django.cbb.library import db, lock
import antirobot.cbb.cbb_django.cbb.models as models
from django.core.management.base import BaseCommand, CommandError
from sqlalchemy import and_


logger = logging.getLogger("cbb.cron.history")


@db.main.use_master()
def flush_expired_blocks(batch_size=None):
    """
    Flushes expired block by batches
    """
    result = {}
    date = datetime.now()
    version_to_str = {
        4: "IPV4",
        6: "IPV6",
        "txt": "Txt",
        "re": "Re",
    }

    for version, RangeClass in models.BLOCK_VERSIONS.items():
        expired_query = RangeClass.query.filter(and_(RangeClass.expire is not None, RangeClass.expire < date))
        count = expired_query.count()
        if not batch_size:
            batch_size = count

        processed_count = 0
        while processed_count < count:
            if count - processed_count < batch_size:
                batch_size = count - processed_count
            expired_blocks = expired_query.limit(batch_size).all()
            if not expired_blocks:
                break
            process_expired_blocks(expired_blocks, version)
            processed_count += batch_size
        result[version_to_str[version]] = processed_count

    return result


def process_expired_blocks(expired_blocks, version):
    """
    Removes expired blocks and tries add its to history.
    Failing that, adds its to limbos.
    """
    # get all uniq groups IDs
    group_ids = set([block.group_id for block in expired_blocks])
    # add batch to history base (or main)
    try:
        HistoryClass = models.HISTORY_VERSIONS[version]
        history_objects = [HistoryClass.from_block(block, unblocked_by="auto", unblock_description="auto expire") for block in expired_blocks]
        with db.history.use_master():
            db.history.session.add_all(history_objects)
            db.history.session.commit()

    except db.DatabaseNotAvailable:
        LimboClass = models.LIMBO_VERSIONS[version]
        limbo_objects = [LimboClass.from_block(block, unblocked_by="auto", unblock_description="auto expire") for block in expired_blocks]
        db.main.session.add_all(limbo_objects)
        db.main.session.commit()
    # remove bulk from main base
    for block in expired_blocks:
        db.main.session.delete(block)
    # update all uniq groups IDs
    for group in models.Group.query.filter(models.Group.id.in_(group_ids)):
        group.update_updated()
    db.main.session.commit()


class Command(lock.LockedCommandMixin, BaseCommand):
    help = "Delete expired blocks and add to history DB ( or limbo table)"

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

        start_message = "Start processing expired blocks"
        logger.info(start_message)
        self.stdout.write(start_message)

        result_str = ", ".join(["{0}={1}".format(k, v) for k, v in flush_expired_blocks(batch_size).items()])

        result_message = "Flushed expired blocks count: {0}".format(result_str)
        logger.info(result_message)
        self.stdout.write(result_message)
