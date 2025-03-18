# coding: utf-8
from __future__ import unicode_literals

import pytest

from ids import exceptions
from ids.services.gap.connector import GapConnector as Connector

pytestmark = pytest.mark.integration


def test_get_gaps_403():
    connector = Connector(user_agent='ids-test', token='shitty')

    with pytest.raises(exceptions.AuthError):
        connector.get(
            resource='gap_list',
            url_vars={
                'period_from': '2011-01-01',
                'period_to': '2012-01-01',
                'login_list': 'teamcity',
            }
        )


def test_get_gaps_ok():
    # токен teamcity должен быть быть заведен в админке gap.test
    connector = Connector(user_agent='ids-test', token='teamcity')

    response = connector.get(
        resource='gap_list',
        url_vars={
            'period_from': '2011-01-01',
            'period_to': '2012-01-01',
            'login_list': 'teamcity',
        }
    )

    assert response.status_code == 200
    assert response.json()['result']
