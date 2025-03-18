# coding: utf-8
from __future__ import unicode_literals

from django.core.management.base import BaseCommand
from django.utils import timezone

from metrics_framework.models import Metric, MetricPoint
from metrics_framework.tasks import compute_metric


class Command(BaseCommand):
    help = 'Starts metric tasks'

    def handle(self, *args, **options):
        for metric in Metric.objects.all():
            last_created_at = MetricPoint.objects.filter(
                metric=metric
            ).order_by('-created_at').values_list('created_at', flat=True).first()
            if not last_created_at or timezone.now() - last_created_at > metric.timedelta:
                compute_metric.delay(metric.slug)
