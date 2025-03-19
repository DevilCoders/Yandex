import time
import requests


class RenewableIAMToken:
    def __init__(self, jwt_token_generator):
        self.jwt_token_generator = jwt_token_generator
        self.cached_token = None
        self.last_updated = None

    def get_iam_token(self):
        if (self.cached_token is None) or ((int(time.time()) - self.last_updated) > 3600.0):
            self.last_updated = int(time.time())
            self.cached_token = self.get_iam_token_from_jwt()

        return self.cached_token

    def get_iam_token_from_jwt(self):
        resp = requests.post(f'https://iam.api.cloud.yandex.net/iam/v1/tokens',
                             headers={'Content-Type': 'application/json'},
                             json={"jwt": self.jwt_token_generator.generate_jwt()})

        return resp.json()['iamToken']

