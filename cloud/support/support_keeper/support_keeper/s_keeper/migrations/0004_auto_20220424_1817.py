# Generated by Django 3.2.9 on 2022-04-24 18:17

from django.db import migrations


class Migration(migrations.Migration):

    dependencies = [
        ('s_keeper', '0003_auto_20220424_1816'),
    ]

    operations = [
        migrations.RenameField(
            model_name='queue_filter',
            old_name='tickets_danger_limit',
            new_name='ts_danger_limit',
        ),
        migrations.RenameField(
            model_name='queue_filter',
            old_name='tickets_warning_limit',
            new_name='ts_warning_limit',
        ),
    ]
