import os
import logging
import typing as tp

from vault_client.instances import Production as VaultClient
from clan_tools.utils.token import file2str, get_token

logger = logging.getLogger(__name__)

SECRET_IDS = {
    'robot-clanalytics-yt': 'sec-01ct0ebtvcs99z0gns9ecg0159',
    'robot-clan-pii-yt': 'sec-01fm06fw1zsqp08cxtyd247tm5',
    'pavelvasilev': 'sec-01f47c9hp0b05jgy1r35rvx5n6'
}


class Vault:
    def __init__(self, token: tp.Optional[str] = None, token_file_path: tp.Optional[str] = None):
        """Client for connection to all tokens of 'robot-clananalytics-yt' or another one.
        More information on https://vault-api.passport.yandex.net/docs/ and https://a.yandex-team.ru/arc/trunk/arcadia/library/python/vault_client

        To get token follow https://oauth.yandex-team.ru/authorize?response_type=token&client_id=ce68fbebc76c4ffda974049083729982

        It is prefered to use YAV_TOKEN environment variable, but you can also save token in .yav_token in your home dir or in token_file_path
        """
        if token is None:
            if 'YAV_TOKEN' in os.environ:
                self._token = os.environ['YAV_TOKEN']
            else:
                self._token = file2str(token_file_path) if token_file_path else get_token(token_file_path='.yav_token')
        else:
            self._token = token
        self._client = VaultClient(authorization=f"OAuth {self._token}", decode_files=True)

    def get_secrets(self, secret_id: str = 'sec-01ct0ebtvcs99z0gns9ecg0159', secret_name: tp.Optional[str] = None,
                    add_to_env: bool = True) -> tp.Optional[tp.Dict[str, str]]:
        '''By default we use robot secrets https://yav.yandex-team.ru/secret/sec-01ct0ebtvcs99z0gns9ecg0159/explore/versions'''

        if secret_name is not None:
            secret_id = SECRET_IDS[secret_name]
        secrets = self._client.get_version(secret_id)['value']

        if add_to_env:
            for secret, secret_value in secrets.items():
                secret_upper = secret.upper()
                if secret_upper not in os.environ:
                    os.environ[secret_upper] = secret_value
            return None
        return secrets
