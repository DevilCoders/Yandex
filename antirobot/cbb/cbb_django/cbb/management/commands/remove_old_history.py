# -*- coding: utf-8 -*-
from antirobot.cbb.cbb_django.cbb.library import db
from datetime import datetime, timedelta
from django.core.management.base import BaseCommand
from antirobot.cbb.cbb_django.cbb.models import HISTORY_VERSIONS


class Command(BaseCommand):
    @db.history.use_master()
    def handle(self, *args, **options):
        for version, HistoryClass in HISTORY_VERSIONS.items():
            month_ago = datetime.now() - timedelta(days=30)
            count = HistoryClass.query.filter(HistoryClass.unblocked_at < month_ago).delete()
            db.history.session.commit()
            self.stdout.write("v" + str(version) + " affected: " + str(count))
