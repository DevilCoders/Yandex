# -*- coding: utf-8 -*-
"""
DBaaS Internal API quota usage
"""

from flask import g
from flask.views import MethodView
from flask_restful import abort

from dbaas_internal_api.utils.quota import SPEC_TO_DB_QUOTA_MAPPING, db_quota_limit_field, db_quota_usage_field

from . import API, marshal
from ..apis import parse_kwargs
from ..apis.support import DISPENSER_USED_ERROR
from ..core.auth import check_auth
from ..core.exceptions import DbaasClientError
from ..utils import config, identity, metadb
from ..utils.config import default_cloud_quota
from .schemas.quota import (
    BatchUpdateQuotaMetricsNoCloudIdRequestSchemaV2,
    BatchUpdateQuotaMetricsRequestSchemaV2,
    GetQuotaDefaultResponseSchemaV2,
    GetQuotaResponseSchemaV2,
)
from .schemas.quota_usage import QuotaUsageSchema


@API.resource('/mdb/v1/quota/<string:cloud_id>')
class UsageInfoV1(MethodView):
    """Get information about quota usage in cloud"""

    @marshal.with_schema(QuotaUsageSchema)
    @check_auth(explicit_action='mdb.quotas.read', entity_type='cloud')
    def get(self, **_):
        """
        Get quota usage in cloud
        """
        g.metadb.commit()
        return g.cloud


@API.resource('/mdb/v2/quota/<string:cloud_id>')
class QuotaInfoV2(MethodView):
    """Get information about quota usage in cloud"""

    @marshal.with_schema(GetQuotaResponseSchemaV2)
    @check_auth(explicit_action='mdb.quotas.read', entity_type='cloud')
    def get(self, **_):
        """
        Get quota usage in cloud
        """
        res = {
            'cloud_id': g.cloud['cloud_ext_id'],
            'metrics': [],
        }

        for spec_name, db_name in SPEC_TO_DB_QUOTA_MAPPING.items():
            res['metrics'].append(
                {
                    'name': spec_name,
                    'usage': g.cloud.get(db_quota_usage_field(db_name)),
                    'limit': g.cloud[db_quota_limit_field(db_name)],
                }
            )
        return res

    @parse_kwargs.with_schema(BatchUpdateQuotaMetricsNoCloudIdRequestSchemaV2)
    @check_auth(explicit_action='mdb.all.support', entity_type='cloud')
    def post(self, cloud_id, metrics):
        """
        Update quota usage in cloud
        """
        _update_quota_limits(cloud_id, metrics)


@API.resource('/mdb/v2/quota')
class QuotaV2(MethodView):
    """Methods for working with cloud quotas"""

    @marshal.with_schema(GetQuotaDefaultResponseSchemaV2)
    def get(self, **_):
        """
        Get default quota values
        """
        defaults = default_cloud_quota()
        res = {'metrics': []}
        for spec_name, db_name in SPEC_TO_DB_QUOTA_MAPPING.items():
            res['metrics'].append(
                {
                    'name': spec_name,
                    'limit': defaults[db_quota_limit_field(db_name)],
                }
            )
        return res

    @parse_kwargs.with_schema(BatchUpdateQuotaMetricsRequestSchemaV2)
    @check_auth(explicit_action='mdb.all.support', entity_type='cloud')
    def post(self, cloud_id, metrics):
        """
        Update quota usage in cloud
        """
        _update_quota_limits(cloud_id, metrics)


def _update_quota_limits(cloud_id, metrics):
    if config.is_dispenser_used():
        raise DbaasClientError(DISPENSER_USED_ERROR)

    cloud, _ = identity.get_cloud_by_ext_id(allow_missing=True, create_missing=True, cloud_id=cloud_id)

    changes = {}
    for metric in metrics:
        metric_name = metric['name']
        new_limit = metric['limit']
        metric_db_name = SPEC_TO_DB_QUOTA_MAPPING[metric_name]
        usage = cloud[db_quota_usage_field(metric_db_name)]
        validate_quota_change(metric_name, usage, new_limit)
        changes[SPEC_TO_DB_QUOTA_MAPPING[metric_name]] = new_limit

    if changes:
        metadb.set_cloud_quota(
            cloud_ext_id=cloud_id,
            cpu=changes.get('cpu', None),
            gpu=changes.get('gpu', None),
            memory=changes.get('memory', None),
            ssd_space=changes.get('ssd_space', None),
            hdd_space=changes.get('hdd_space', None),
            clusters=changes.get('clusters', None),
        )
        g.metadb.commit()


def validate_quota_change(name, current_used, new_limit):
    """Validate cloud quota change"""

    if new_limit < current_used:
        abort(
            422,
            message="{name} quota would become less than {name} used".format(name=name),
        )


def _bytes_to_gb(in_bytes):
    return in_bytes / (1024 * 1024 * 1024)
