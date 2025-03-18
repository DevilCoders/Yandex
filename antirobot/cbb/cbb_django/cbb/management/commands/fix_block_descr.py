# -*- coding: utf-8 -*-
from antirobot.cbb.cbb_django.cbb.library import db
from datetime import datetime, timedelta
from django.core.management.base import NoArgsCommand
from antirobot.cbb.cbb_django.cbb.models import HISTORY_VERSIONS
from sqlalchemy import desc


class Command(NoArgsCommand):

    @db.history.use_master()
    def handle_noargs(self, **options):
        for version in (4, 6, 'txt'):
            HistoryClass = HISTORY_VERSIONS[version]
            month_ago = datetime.now() - timedelta(days=31)
            # Берем записи на удаление младше месяца, у которых пустое поле "описание блокировки"
            candidates = HistoryClass.query.filter(HistoryClass.operation_date > month_ago).filter_by(operation='del', block_descr='')
            self.stdout.write('v' + str(version) + ' candidates:' + str(candidates.count()))

            affected_count = 0
            for candidate in candidates:
                if version == 'txt':
                    add_query = HistoryClass.query.filter_by(rng_txt=candidate.rng_txt)
                else:
                    add_query = HistoryClass.query.filter_by(rng_start=candidate.rng_start, rng_end=candidate.rng_end)
                # Находим для такого поля запись на добавление
                add = add_query.filter_by(group_id=candidate.group_id).order_by(desc(HistoryClass.operation_date)).limit(1).first()
                if add and add.operation_descr:
                    # И берем у нее из поля "описание операции" поле "описание блокировки"
                    candidate.block_descr = add.operation_descr
                    affected_count += 1
            db.history.session.commit()
            self.stdout.write('v' + str(version) + ' affected: ' + str(affected_count))
