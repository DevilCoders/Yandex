# -*- coding: utf-8 -*-
# Generated by Django 1.10 on 2017-07-31 10:43
from __future__ import unicode_literals

from django.db import migrations, models


class Migration(migrations.Migration):

    dependencies = [
        ('dir_data_sync', '0004_add_lang_to_org'),
    ]

    operations = [
        migrations.AlterField(
            model_name='organization',
            name='label',
            field=models.CharField(help_text='\u0423\u043d\u0438\u043a\u0430\u043b\u044c\u043d\u044b\u0439 \u0447\u0435\u043b\u043e\u0432\u0435\u043a\u043e-\u0447\u0438\u0442\u0430\u0435\u043c\u044b\u0439 id \u043e\u0440\u0433\u0430\u043d\u0438\u0437\u0430\u0446\u0438\u0438', max_length=1000),
        ),
    ]
