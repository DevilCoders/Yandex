"""Vault config module"""
import logging
from library.python.vault_client.instances import Production as VaultClient

# pylint: disable=invalid-name
logger = logging.getLogger(__name__)


def _make_secret_name(login, cid):
    return f'mdb-{cid}-{login}'


class Vault:
    """Vault config class"""

    def __init__(self):
        self.token = None
        self.enabled = False

    def init_vault(self, config):
        """Method to throw constants from file to vault config"""

        self.token = config.get('VAULT_OAUTH_TOKEN', '')
        self.enabled = config.get('STORE_PASSWORD_IN_VAULT', False)
        self.client = None
        if self.enabled:
            self.client = VaultClient(authorization=f'OAuth {self.token}', decode_files=True)

    def store_password(self, login, password, cid, uid):
        if not self.enabled:
            raise RuntimeError('Store password in vault is disabled in config')
        secret_uuid = self._find_existing_secret_uuid(login, cid)
        if not secret_uuid:
            secret_uuid = self.client.create_secret(_make_secret_name(login, cid))
            logger.info('Created secret %s', secret_uuid)
        version_uuid = self.client.create_secret_version(secret_uuid, {'password': password})
        logger.info('Created version %s', version_uuid)
        self.client.add_user_role_to_secret(secret_uuid, 'owner', uid=uid)
        return secret_uuid, version_uuid

    def _find_existing_secret_uuid(self, login, cid):
        secret_name = _make_secret_name(login, cid)
        logger.info('Searching secret with name %s', secret_name)
        secrets = self.client.list_secrets(
            order_by='created_at', asc=False, yours=True, query=secret_name, query_type='exact'
        )
        for secret in secrets:
            logger.info('Found secret %s', secret['uuid'])
            return secret['uuid']
        logger.info('Secret %s not found', secret_name)


VAULT = Vault()
