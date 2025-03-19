import base64

import pytest

from yc_auth_token import Scope, SignedToken, Token


@pytest.fixture(scope="module")
def raw_token():
    token = Token()

    principal = token.principal
    principal.id = "6289bb40-35ea-4844-b44b-4070a79469df"
    principal.name = "test_user"

    scope = token.scope
    scope.type = Scope.Type.Value("CLOUD")
    scope.id = "8bfb4803-9bd3-4544-93bd-9c62e966abcf"
    return token


@pytest.fixture(scope="module")
def signed_token(raw_token):
    signed_token = SignedToken()
    signed_token.token = raw_token.SerializeToString()
    return signed_token.SerializeToString()


@pytest.fixture(scope="module")
def encoded_token(signed_token):
    return base64.urlsafe_b64encode(signed_token).rstrip(b'=')


@pytest.fixture(scope="module")
def raw_token_w_signature_disabled():
    token = Token()

    principal = token.principal
    principal.id = "6289bb40-35ea-4844-b44b-4070a79469df"
    principal.name = "test_user"

    scope = token.scope
    scope.type = Scope.Type.Value("CLOUD")
    scope.id = "8bfb4803-9bd3-4544-93bd-9c62e966abcf"

    token.disable_signature_check = True
    return token


@pytest.fixture(scope="module")
def signed_token_w_signature_disabled(raw_token_w_signature_disabled):
    signed_token = SignedToken()
    signed_token.token = raw_token_w_signature_disabled.SerializeToString()
    return signed_token.SerializeToString()


@pytest.fixture(scope="module")
def encoded_token_w_signature_disabled(signed_token_w_signature_disabled):
    return base64.urlsafe_b64encode(signed_token_w_signature_disabled).rstrip(b'=')
