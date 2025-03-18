# coding: utf-8

from __future__ import unicode_literals

from ids.services.staff import connector as staff_connector
from ids.lib import paging_api
from ids.lib import pagination

from django_ids.connector import http


class StaffConnector(http.HttpConnector):

    service_name = staff_connector.StaffConnector.service_name
    url_patterns = staff_connector.StaffConnector.url_patterns


def get_result_set(auth, resource, connector_params=None, **request_params):
    connector_params = connector_params or {}
    connector = StaffConnector(auth=auth, **connector_params)
    fetcher = paging_api.PagingApiFetcher(
        connector=connector,
        resource=resource,
        **request_params
    )
    return pagination.ResultSet(fetcher=fetcher)
