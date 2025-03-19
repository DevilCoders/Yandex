"""Common models for QuotaService."""

from typing import Dict

from yc_common.clients.models.base import BaseListModel

from yc_common import fields
from yc_common.models import Model


class MetricLimit(Model):
    name = fields.StringType(required=True)
    limit = fields.IntType(required=True, min_value=0)


class QuotaMetric(Model):
    name = fields.StringType(required=True)
    value = fields.IntType(required=True)
    limit = fields.IntType(required=True)
    soft_limit = fields.IntType(required=False)  # FIXME(zasimov-a): mark as required after transition period


class _QuotaMetrics(Model):
    metrics = fields.ListType(fields.ModelType(QuotaMetric), required=True)

    def get_usage(self) -> Dict[str, int]:
        usage = {}
        for quota_metric in self.metrics:
            usage[quota_metric.name] = usage.get(quota_metric.name, 0) + quota_metric.value
        return usage

    def get_limits(self) -> Dict[str, int]:
        limits = {}
        for quota_metric in self.metrics:
            limits[quota_metric.name] = quota_metric.limit
        return limits

    def get_soft_limits(self) -> Dict[str, int]:
        return {quota_metric.name: quota_metric.soft_limit for quota_metric in self.metrics}


class Quota(_QuotaMetrics):
    cloud_id = fields.StringType(required=True)


class Quotas(BaseListModel):
    quotas = fields.ListType(fields.ModelType(Quota), required=True)


class FolderQuota(_QuotaMetrics):
    folder_id = fields.StringType(required=True)
