# -*- coding: utf-8 -*-
from __future__ import unicode_literals

from django_abc_data import core
from django_abc_data.compat import shared_task


@shared_task
def sync_services(*args, **kwargs):
    core.sync_services(*args, **kwargs)
