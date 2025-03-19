"""
Register mocks
"""

from unittest.mock import MagicMock

from dbaas_internal_api.health.health import HealthInfo


def mock_feature_flags(mocker):
    """
    Mock feature flags

    You have all of them
    """
    super_feature_flag = MagicMock()
    super_feature_flag.__contains__.return_value = True
    mocker.patch('dbaas_internal_api.utils.feature_flags.get_feature_flags', return_value=super_feature_flag)


def mock_valid_versions(package, mocker):
    """
    Mock valid versions
    """
    magic_mock = MagicMock()
    magic_mock.__contains__.return_value = True
    mocker.patch(package + '.get_valid_versions', return_value=magic_mock)


def mock_logs(mocker):
    """
    Mock logs

    Cause them try access current Flask app
    """
    mocker.patch('dbaas_internal_api.utils.logs.get_background_logger_name', return_value='tests')


def mock_mdb_health_from_memory(mocker, ret: HealthInfo):
    """
    Mock MDB Health client's loading of health for fqdns

    Cause them try access current Flask app
    """
    mocker.patch('dbaas_internal_api.health.health.MDBHealthProviderHTTP.get_hosts_health', return_value=ret)
