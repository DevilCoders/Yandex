"""Utility functions."""
import logging
import os
import random
import smtplib
import string
from email.message import EmailMessage

import flask
from nacl import utils
from nacl.encoding import URLSafeBase64Encoder as encoder
from nacl.public import Box, PrivateKey, PublicKey
from blackbox import JsonBlackbox as Blackbox
from tvmauth import BlackboxTvmId

from .tvm_constants import TVM_CONSTANTS
from .vault import VAULT

# pylint: disable=invalid-name
logger = logging.getLogger(__name__)

IDM_ORIGIN = 'idm'

GRANTS = {
    'reader': {
        'name': {
            'ru': 'Чтение',
            'en': 'Reader',
        },
    },
    'writer': {
        'name': {
            'ru': 'Редактирование',
            'en': 'Writer',
        },
    },
    'responsible': {
        'name': {
            'ru': 'Ответственный',
            'en': 'Responsible',
        },
    },
}
MDB_ADMIN_EMAIL = 'mdb-admin@yandex-team.ru'
EMAIL_TEMPLATE = """
Greetings from MDB (DBaaS)!

Your current password for cluster "{cname}" ({cid}) has expired.
Your new password is:
{password}

Use this password to log into "{cname}" ({cid}) with username "{login}".
It is highly recommended that you use .pgpass file to store it.
"""
EMAIL_TEMPLATE_VAULT = """
Greetings from MDB (DBaaS)!

Your current password for cluster "{cname}" ({cid}) has expired.
Your new password link is:
https://yav.yandex-team.ru/secret/{secret_id}/explore/version/{secret_version_id}

Use this password to log into "{cname}" ({cid}) with username "{login}".
It is highly recommended that you use .pgpass file to store it.
"""


def get_login_uid(login):
    """Returns login passport uid"""
    client = Blackbox(
        url='http://blackbox.yandex-team.ru/blackbox',
        tvm2_client_id=TVM_CONSTANTS.client_id,
        tvm2_secret=TVM_CONSTANTS.client_secret,
        blackbox_client=BlackboxTvmId.ProdYateam,
    )
    result = client.userinfo(login=login, userip='127.0.0.1')
    return result['users'][0]['id']


def send_password_email(login, password, cluster, host):
    """Send an email to the user."""
    email_to = '{}@yandex-team.ru'.format(login)
    message = EmailMessage()
    message['Subject'] = f'New password for {login} at {cluster.name} ({cluster.cid}) in MDB (DBaaS)'
    message['From'] = MDB_ADMIN_EMAIL
    message['To'] = email_to
    if VAULT.enabled:
        secret_id, secret_version = VAULT.store_password(login, password, cluster.cid, get_login_uid(login))
        message.set_content(
            EMAIL_TEMPLATE_VAULT.format(
                cname=cluster.name, cid=cluster.cid, login=login, secret_id=secret_id, secret_version_id=secret_version
            )
        )
    else:
        message.set_content(EMAIL_TEMPLATE.format(cname=cluster.name, cid=cluster.cid, login=login, password=password))
    with smtplib.SMTP(host) as server:
        server.send_message(message)
        logger.info('Sent email with new password to %s(%s) for %s (%s)', login, email_to, cluster.name, cluster.cid)


def generate_password(length=32):
    """Generate a random password."""
    # Passwords generations should be fixed in MDB-5204
    chars = string.ascii_letters + string.digits
    password = ''.join(random.choice(chars) for _ in range(length))  # nosec
    return password


def encrypt(config, data):
    """Encrypt data and return dict with embedded encryption version."""
    pub_key = PublicKey(config['client_pub_key'].encode('utf-8'), encoder)
    sec_key = PrivateKey(config['api_sec_key'].encode('utf-8'), encoder)
    box = Box(sec_key, pub_key)

    encrypted = encoder.encode(box.encrypt(data.encode('utf-8'), utils.random(box.NONCE_SIZE))).decode('utf-8')

    return {'encryption_version': 1, 'data': encrypted}


def is_hiding():
    """Check if should hide from the balancer."""
    hide_flag = flask.current_app.config['HIDE_FLAG']
    return os.path.exists(hide_flag)
