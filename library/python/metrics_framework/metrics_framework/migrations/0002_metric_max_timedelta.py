# -*- coding: utf-8 -*-
from __future__ import unicode_literals

from django.db import migrations, models
import datetime


class Migration(migrations.Migration):

    dependencies = [
        ('metrics_framework', '0001_initial'),
    ]

    operations = [
        migrations.AddField(
            model_name='metric',
            name='max_timedelta',
            field=models.DurationField(default=datetime.timedelta(0, 1800)),
            preserve_default=False,
        ),
    ]
