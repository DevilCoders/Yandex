from antirobot.cbb.cbb_django.cbb.library import db
from django.core.management.base import BaseCommand

from antirobot.cbb.cbb_django.cbb.models import BLOCK_VERSIONS


@db.main.use_master(commit_on_exit=True)
def emptify_group(group_id, dry_run):
    for RangeClass in BLOCK_VERSIONS.values():
        for block in RangeClass.query.filter(RangeClass.group_id==group_id).all():
            if not dry_run:
                db.main.session.delete(block)
                print(f"Block {block} has been deleted for group {group_id}")
            else:
                print(f"Block {block} will be deleted for group {group_id}, nothing has been done due to dry_run option")

    if not dry_run:
        db.main.session.commit()


class Command(BaseCommand):
    help = "Delete all ranges for group_id"

    def add_arguments(self, parser):
        parser.add_argument(
            "--group",
            type=int,
            help="group_id",
        )
        parser.add_argument(
            "--dry-run",
            action="store_true",
            help="Do not actually delete anything, just log actions",
        )

    def handle(self, *args, **options):
        print(options["group"])
        emptify_group(options["group"], options["dry_run"])
        print("Done")
