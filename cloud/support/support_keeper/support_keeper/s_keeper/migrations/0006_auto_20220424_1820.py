# Generated by Django 3.2.9 on 2022-04-24 18:20

from django.db import migrations, models
import uuid


class Migration(migrations.Migration):

    dependencies = [
        ('s_keeper', '0005_auto_20220424_1819'),
    ]

    operations = [
        migrations.RemoveField(
            model_name='support_unit',
            name='id',
        ),
        migrations.AddField(
            model_name='support_unit',
            name='u_id',
            field=models.UUIDField(default=uuid.uuid4, editable=False, primary_key=True, serialize=False),
        ),
    ]
