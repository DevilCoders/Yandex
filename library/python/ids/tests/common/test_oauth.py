# coding: utf-8

import pytest

from ids.exceptions import BackendError
from ids.helpers import oauth


@pytest.mark.integration
def test_get_token():
    uid = 1120000000014341
    oauth_id = 'fd9f477e76e74e6ab82a7acaa4de7b32'
    oauth_secret = '95485837153640608777288fd6e3b4e5'

    assert oauth.get_token(uid, oauth_id, oauth_secret) is not None
    assert oauth.get_token_by_uid(oauth_id, oauth_secret, uid) is not None


@pytest.mark.integration
def test_get_token_invalid_client():
    uid = 1120000000014341
    oauth_id = '123'
    oauth_secret = 'abc'

    with pytest.raises(BackendError):
        oauth.get_token(uid, oauth_id, oauth_secret)
    with pytest.raises(BackendError):
        oauth.get_token_by_uid(oauth_id, oauth_secret, uid)
