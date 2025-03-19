"""
Fixtures suddenly
"""

import pytest
from flask import Flask
from flask_appconfig import AppConfig
from nacl.encoding import URLSafeBase64Encoder as encoder
from nacl.public import PrivateKey


def _generate_temp_crypto_keys():
    private_key = PrivateKey.generate()
    public_key = private_key.public_key
    return {
        'client_public_key': encoder.encode(bytes(private_key)).decode('utf-8'),
        'api_secret_key': encoder.encode(bytes(public_key)).decode('utf-8'),
    }


@pytest.fixture
def app_config():
    """
    Allows testing code to user current_app.config
    """
    app = Flask("dbaas_internal_api")
    AppConfig(app, None)
    app.config['CRYPTO'].update(_generate_temp_crypto_keys())

    with app.app_context():
        yield app.config
