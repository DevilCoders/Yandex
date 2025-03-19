import mock
import pytest

from gauthling_daemon import GauthlingClient
from yc_auth.authorization import authz, authorize
from yc_auth.exceptions import UnauthorizedOperationError


@mock.patch.object(GauthlingClient, "authz", autospec=True)
def test_authz(authz_mock, signed_token):
    authz_mock.return_value = True

    gauthling_client = GauthlingClient("fake_url")
    access_service_client = None

    def authorize(action):
        return authz(
            lambda: gauthling_client,
            lambda: access_service_client,
            action,
            token_extractor=lambda: signed_token,
            folder_id_extractor=lambda: "project_id",
        )

    with pytest.raises(ValueError) as exc_info:
        @authorize(action="do_something")
        def dummy_handler():
            pass

    assert exc_info.value.message == "Gauthling is no longer supported."


@mock.patch.object(GauthlingClient, "authz", autospec=True)
def test_authorize(authz_mock, signed_token):
    authz_mock.return_value = True

    gauthling_client = GauthlingClient("fake_url")
    access_service_client = None
    action = "test_action"

    with pytest.raises(ValueError) as exc_info:
        authz_mock.return_value = False
        with pytest.raises(UnauthorizedOperationError) as exc_info:
            authorize(gauthling_client, access_service_client, action, signed_token, folder_id="test_project")

    assert exc_info.value.message == "Gauthling is no longer supported."
