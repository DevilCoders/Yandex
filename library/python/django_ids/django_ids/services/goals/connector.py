# coding: utf-8

from __future__ import unicode_literals

from ids.services.goals import connector as goals_connector
from ids.lib import cia
from ids.lib import pagination

from django_ids.connector import http


class GoalsConnector(http.HttpConnector):

    service_name = goals_connector.GoalsConnector.service_name
    url_patterns = goals_connector.GoalsConnector.url_patterns


def get_result_set(auth, resource, connector_params=None, **request_params):
    connector_params = connector_params or {}
    connector = GoalsConnector(auth=auth, **connector_params)
    fetcher = cia.CIAFetcher(
        connector=connector,
        resource=resource,
        **request_params
    )
    return pagination.ResultSet(fetcher=fetcher)
