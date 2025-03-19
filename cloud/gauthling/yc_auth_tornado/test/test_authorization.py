import mock
import pytest
from tornado.concurrent import Future

from yc_auth.exceptions import UnauthorizedOperationError
from yc_auth_tornado.authorization import authorize


@pytest.mark.gen_test
def test_authorize(signed_token):
    gauthling_client = mock.MagicMock()
    positive = Future()
    positive.set_result(True)
    gauthling_client.authz.return_value = positive

    access_service_client = None

    with pytest.raises(ValueError) as exc_info:
        action = "test_action"
        yield authorize(gauthling_client, access_service_client, action, signed_token, folder_id="test_project")

    assert exc_info.value.message == "Gauthling is no longer supported."

    with pytest.raises(ValueError) as exc_info:
        negative = Future()
        negative.set_result(False)
        gauthling_client.authz.return_value = negative
        with pytest.raises(UnauthorizedOperationError) as exc_info:
            yield authorize(gauthling_client, access_service_client, action, signed_token, folder_id="test_project")

    assert exc_info.value.message == "Gauthling is no longer supported."
