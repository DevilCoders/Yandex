# coding: utf-8
from __future__ import unicode_literals

import logging

from celery import shared_task
from django.conf import settings
from django.db import transaction
from django.utils import timezone
import statface_client

from metrics_framework.lock import Lock
from metrics_framework.models import MetricPoint, Metric


log = logging.getLogger(__name__)


def update_statface_config(report, metric_point):
    should_be_uploaded = False
    config = report.download_config()
    fields = [next(iter(x)) for x in config['fields']]
    for field in metric_point.values.values_list('slug', flat=True):
        if field not in fields:
            should_be_uploaded = True
            config['fields'].append({field: {'title': field}})
            config['user_config']['measures'].append({field: 'number'})
    if should_be_uploaded:
        report.upload_config(config)


def push_to_statface(metric_points):
    statface = statface_client.StatfaceClient(
        host=settings.STATFACE_HOST,
        oauth_token=settings.STATFACE_OAUTH_TOKEN,
    )
    metric_point = metric_points[0]
    remote_report = statface.get_report(metric_point.metric.report_path)

    fielddate = metric_point.created_at.replace(second=0, microsecond=0)
    if fielddate.tzinfo is not None:
        fielddate = fielddate.astimezone(timezone.pytz.timezone(settings.TIME_ZONE))
    fielddate = fielddate.strftime('%F %T')
    data_to_upload = []
    for metric_point in metric_points:
        data = {'fielddate': fielddate}
        if metric_point.context is not None:
            data.update(metric_point.context)
        for value in metric_point.values.all():
            data[value.slug] = value.value
        data_to_upload.append(data)

    update_statface_config(remote_report, metric_point)
    remote_report.upload_data(scale='minutely', data=data_to_upload)


@shared_task
def compute_metric(metric_slug):
    with Lock(metric_slug) as is_locked:
        if not is_locked:
            return
        metric = Metric.objects.get(slug=metric_slug)
        f, value_model = settings.METRIC_SLUG_MAPPING[metric_slug]
        values = f()
        if not values:
            log.warning('Metric %s return empty value', metric_slug)
            return
        if 'context' not in values[0]:
            # Получили значения одной точки, обернём в список
            values = [values]
        metric_points = process_values(metric, value_model, values)
        if metric.is_exportable:
            push_to_statface(metric_points)


def process_values(metric, value_model, values):
    metric_points = []
    for value in values:
        metric_points.append(process_one_row(metric, value_model, value))
    return metric_points


def process_one_row(metric, ValueModel, values):
    if 'context' in values:
        context = values['context']
        data = values['values']
    else:
        context = None
        data = values
    with transaction.atomic():
        metric_point = MetricPoint.objects.create(metric=metric, context=context)
        for value in data:
            ValueModel.objects.create(metric_point=metric_point, **value)
    return metric_point
