import base64
import datetime
import os
import sys
import uuid

from cryptography.hazmat.backends import default_backend
from cryptography.hazmat.primitives import hashes
from cryptography.hazmat.primitives import serialization
from cryptography.hazmat.primitives.asymmetric import padding
from yc_auth_token import AuthType
from yc_auth_token import Principal
from yc_auth_token import Scope
from yc_auth_token import SignedToken
from yc_auth_token import Token
from yc_auth_token import TokenVersion
from yc_common import config

if sys.version_info > (3, 3):
    def timestamp(dt):
        return dt.timestamp()
else:
    import time

    def timestamp(dt):
        return time.mktime(dt.timetuple()) + dt.microsecond / 1000000.0

_PRIVATE_KEY = None


def load_key():
    global _PRIVATE_KEY
    if _PRIVATE_KEY is not None:
        return _PRIVATE_KEY
    path = os.path.expanduser(config.get_value("marketplace.service_account.private_key_path"))
    with open(path, "rb") as f:
        _PRIVATE_KEY = f.read()
        return _PRIVATE_KEY


def create_service_account_token(
        service_account_id,
        service_account_login,
        folder_id,
        key_id,
        private_key,
        is_system=False,
        expires_in=datetime.timedelta(hours=12),
        cloud_id=None,
):
    token = Token()
    token.version = TokenVersion.Value("v2")
    token.id = uuid.uuid4().hex
    issue_time = datetime.datetime.now()
    expire_time = issue_time + expires_in
    token.issued_at = int(timestamp(issue_time))
    token.expires_at = int(timestamp(expire_time))
    token.auth_type = AuthType.Value("SERVICE")

    token.principal.id = service_account_id
    token.principal.name = service_account_login
    token.principal.type = Principal.Type.Value("SYSTEM") if is_system else Principal.Type.Value("SERVICE")
    token.principal.folder_id = folder_id

    token.scope.type = Scope.Type.Value("GLOBAL")

    token.disable_signature_check = True

    token_bytes = token.SerializeToString()

    private_key = serialization.load_pem_private_key(
        private_key,
        password=None,
        backend=default_backend()
    )
    signature = private_key.sign(token_bytes, padding.PKCS1v15(), hashes.SHA384())

    signed_token = SignedToken()
    signed_token.token = token_bytes
    signed_token.sign_key.id = key_id
    signed_token.sign_key.alg = 2  # OpenSSLRSA4096Sha384
    signed_token.sign = signature
    encoded_token = base64.urlsafe_b64encode(signed_token.SerializeToString()).rstrip(b'=')

    return encoded_token


def service_token():
    key = load_key()
    return create_service_account_token(
        service_account_id=config.get_value("marketplace.service_account.service_account_id"),
        service_account_login=config.get_value("marketplace.service_account.service_account_login"),
        folder_id=config.get_value("marketplace.service_account.folder_id"),
        cloud_id=config.get_value("marketplace.service_account.cloud_id"),
        key_id=config.get_value("marketplace.service_account.key_id"),
        private_key=key,
    )
