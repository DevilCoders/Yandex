# -*- coding: utf-8 -*-
"""
DBaaS Internal API quota related schemas
"""
from marshmallow import Schema
from marshmallow.fields import Float, Int, Nested

from .fields import Str


class QuotaMetricSchema(Schema):
    """
    Cloud quota metric schema
    """

    name = Str(required=True, attribute='name')
    usage = Float(required=True, attribute='usage')
    limit = Int(required=True, attribute='limit')


class MetricLimitSchema(Schema):
    """
    Cloud quota limit schema
    """

    name = Str(required=True, attribute='name')
    limit = Int(required=True, attribute='limit')


class GetQuotaResponseSchemaV2(Schema):
    """
    Cloud quota limits and usage response
    """

    cloud_id = Str(required=True)
    metrics = Nested(QuotaMetricSchema, many=True)


class BatchUpdateQuotaMetricsRequestSchemaV2(Schema):
    """
    Update cloud quota schema
    """

    cloud_id = Str(required=True)
    metrics = Nested(MetricLimitSchema, many=True, only=('name', 'limit'))


class BatchUpdateQuotaMetricsNoCloudIdRequestSchemaV2(Schema):
    """
    Update cloud quota schema without cloud_id in body
    """

    metrics = Nested(MetricLimitSchema, many=True, only=('name', 'limit'))


class GetQuotaDefaultResponseSchemaV2(Schema):
    """
    Default quota schema
    """

    metrics = Nested(
        MetricLimitSchema,
        many=True,
    )
