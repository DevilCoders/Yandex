# -*- coding: utf-8 -*-
from __future__ import unicode_literals

from django.db import migrations, models


class Migration(migrations.Migration):

    dependencies = [
    ]

    operations = [
        migrations.CreateModel(
            name='Metric',
            fields=[
                ('slug', models.CharField(max_length=255, unique=True, serialize=False, primary_key=True)),
                ('timedelta', models.DurationField()),
                ('is_exportable', models.BooleanField()),
            ],
        ),
        migrations.CreateModel(
            name='MetricPoint',
            fields=[
                ('id', models.AutoField(verbose_name='ID', serialize=False, auto_created=True, primary_key=True)),
                ('created_at', models.DateTimeField(auto_now=True, db_index=True)),
                ('metric', models.ForeignKey(to='metrics_framework.Metric', on_delete=models.CASCADE)),
            ],
        ),
    ]
