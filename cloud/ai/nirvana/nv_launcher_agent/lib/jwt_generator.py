import time
import jwt

from cloud.ai.nirvana.nv_launcher_agent.lib.config import Config


class JWTTokenGenerator:
    def __init__(self, key_id, service_account_id, private_key_path):
        self.service_account_id = service_account_id
        self.key_id = key_id
        with open(private_key_path, 'r') as private:
            self.private_key = private.read()

    def generate_jwt(self):
        now = int(time.time())

        payload = {
            'aud': 'https://iam.api.cloud.yandex.net/iam/v1/tokens',
            'iss': self.service_account_id,
            'iat': now,
            'exp': now + 3600}

        encoded_token = jwt.encode(
            payload,
            self.private_key,
            algorithm='PS256',
            headers={'kid': self.key_id})

        return encoded_token


if __name__ == '__main__':
    gen = JWTTokenGenerator(
        Config.get_cloud_service_account_key_id(),
        Config.get_cloud_service_account_id(),
        Config.get_cloud_service_account_private_key_pem_file()
    )

    print(gen.generate_jwt())
