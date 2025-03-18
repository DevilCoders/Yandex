# -*- coding: utf-8 -*-

from __future__ import unicode_literals

from django.db import migrations, models


class Migration(migrations.Migration):

    dependencies = [
        ('dir_data_sync', '0010_add_stats_to_every_org'),
    ]

    operations = [
        migrations.AddField(
            model_name='organization',
            name='status',
            field=models.CharField(choices=[('enabled', 'enabled'), ('disabled', 'disabled')], default='enabled', help_text='\u0421\u0442\u0430\u0442\u0443\u0441 \u043f\u043e\u0434\u043a\u043b\u044e\u0447\u0435\u043d\u043d\u043e\u0441\u0442\u0438 \u043e\u0440\u0433\u0430\u043d\u0438\u0437\u0430\u0446\u0438\u0438', max_length=20),
            preserve_default=False,
        ),
    ]
