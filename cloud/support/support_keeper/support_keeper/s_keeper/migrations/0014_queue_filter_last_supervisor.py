# Generated by Django 3.2.9 on 2022-05-15 16:15

from django.db import migrations, models


class Migration(migrations.Migration):

    dependencies = [
        ('s_keeper', '0013_auto_20220515_0948'),
    ]

    operations = [
        migrations.AddField(
            model_name='queue_filter',
            name='last_supervisor',
            field=models.CharField(blank=True, max_length=1024),
        ),
    ]
