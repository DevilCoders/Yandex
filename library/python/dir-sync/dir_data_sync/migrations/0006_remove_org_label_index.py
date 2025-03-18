# coding: utf-8

from __future__ import unicode_literals

from django.conf import settings
from django.db import migrations

"""
Миграция 0005_auto_20170731_1043 не удалила индексы поля label из базы:

$ wiki_manage sqlmigrate dir_data_sync 0005_auto_20170731_1043
BEGIN;
--
-- Alter field label on organization
--

COMMIT;

Индексы остались:

$ \d dir_data_sync_organization;
"dir_data_sync_organization_label_key" UNIQUE CONSTRAINT, btree (label),
"dir_data_sync_organization_label_050fe3aa_like" btree (label varchar_pattern_ops)

Поэтому убираем индексы явно данной миграцией.
"""


class Migration(migrations.Migration):
    dependencies = [
        ('dir_data_sync', '0005_auto_20170731_1043'),
    ]

    # Миграция работает только на postgresql
    if all(
        ['postgresql' in settings.DATABASES[database]['ENGINE'] for database in settings.DATABASES]
    ):
        operations = [
            migrations.RunSQL(
                'ALTER TABLE dir_data_sync_organization DROP CONSTRAINT IF EXISTS dir_data_sync_organization_label_key;'
            ),
            migrations.RunSQL(
                'DROP INDEX IF EXISTS dir_data_sync_organization_label_key;',
                reverse_sql='CREATE UNIQUE INDEX dir_data_sync_organization_label_key ON dir_data_sync_organization USING btree (label);'
            ),
            migrations.RunSQL(
                'DROP INDEX IF EXISTS dir_data_sync_organization_label_050fe3aa_like;',
                reverse_sql='CREATE INDEX dir_data_sync_organization_label_050fe3aa_like ON dir_data_sync_organization USING btree (label varchar_pattern_ops);'
            ),
        ]
    else:
        operations = []
