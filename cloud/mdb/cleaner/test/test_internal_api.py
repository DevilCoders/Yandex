from unittest.mock import Mock
from cloud.mdb.cleaner.internal.internal_api import get_int_api_session
from .test_config import get_config


def test_get_int_api_session(mocker):
    config = get_config()
    iam_token = 'some-token'
    get_token = mocker.patch('cloud.mdb.cleaner.internal.auth.IamJwt.get_token')
    get_token.side_effect = [
        iam_token,
    ]
    session = Mock()
    requests = mocker.patch('cloud.mdb.cleaner.internal.internal_api.requests')
    requests.session.side_effect = [session]
    result = get_int_api_session(config)
    get_token.assert_called_once()
    assert session is result
    assert result.headers == {
        'Content-Type': 'application/json',
        'Accept': 'application/json',
        'X-YaCloud-SubjectToken': iam_token,
    }
    assert result.verify
