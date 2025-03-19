import requests
import base64

from cloud.ai.nirvana.nv_launcher_agent.lib.jwt_generator import JWTTokenGenerator
from cloud.ai.nirvana.nv_launcher_agent.lib.renewable_iam_token_generator import RenewableIAMToken
from cloud.ai.nirvana.nv_launcher_agent.lib.config import Config


class KeyManagementService:
    def __init__(
        self,
        key_id: str,
        cloud_service_account_key_id: str,
        cloud_service_account_id: str,
        service_account_private_key_pem_file: str
    ):
        self.key_id = key_id
        self.iam_token_provider = RenewableIAMToken(
            JWTTokenGenerator(
                cloud_service_account_key_id,
                cloud_service_account_id,
                service_account_private_key_pem_file
            )
        )

    def encrypt(self, data: str):
        base64_data = base64.b64encode(str.encode(data)).decode("ascii")

        resp = requests.post(f'https://kms.yandex/kms/v1/keys/{self.key_id}:encrypt',
                             timeout=7200.0,
                             headers={'Content-Type': 'application/json',
                                      'Authorization': f'Bearer {self.iam_token_provider.get_iam_token()}'},
                             json={"plaintext": base64_data})

        content = resp.json()

        return content['ciphertext']

    def decrypt(self, encrypted_key: str):
        resp = requests.post(f'https://kms.yandex/kms/v1/keys/{self.key_id}:decrypt',
                             timeout=7200.0,
                             headers={'Content-Type': 'application/json',
                                      'Authorization': f'Bearer {self.iam_token_provider.get_iam_token()}'},
                             json={"ciphertext": encrypted_key})

        content = resp.json()
        base64_data = content['plaintext']
        real_data = base64.b64decode(str.encode(base64_data)).decode("ascii")
        return real_data


if __name__ == '__main__':
    service = KeyManagementService(
        Config.get_kms_key_id(),
        Config.get_cloud_service_account_key_id(),
        Config.get_cloud_service_account_id(),
        Config.get_cloud_service_account_private_key_pem_file()
    )

    s = "hello"
    print(f'Str    : {s}')
    enc = service.encrypt(s)
    print(f'Encoded: {enc}')
    decrypted = service.decrypt(enc)
    print(f'Decoded: {decrypted}')
    assert decrypted == s
