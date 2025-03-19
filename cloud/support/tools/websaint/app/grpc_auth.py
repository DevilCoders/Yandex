import app
from yandex.cloud.priv.iam.v1 import iam_token_service_pb2_grpc as iam_token_service
from yandex.cloud.priv.iam.v1 import iam_token_service_pb2 as iam_token_service_pb2
import jwt
import time
from .quotaservice.constants import iam_url
from .quotaservice.helpers import get_grpc_channel
import uuid


class AuthService:
    def __init__(self):
        self.expire = int(time.time())
        self.token = ''

    def get_iam_token(self):

        account_id = app.Config.account_id
        key_id = app.Config.key_id
        private_key = app.Config.private_key
        # ToDo replace to Config
        aud = 'https://iam.api.cloud.yandex.net/iam/v1/tokens'
        now = int(time.time())

        payload = {
            'aud': aud,
            'iss': account_id,
            'iat': now,
            'exp': now + 360}

        encoded_token = jwt.encode(
            payload,
            private_key,
            algorithm='PS256',
            headers={'kid': key_id})

        iam_endpoint = iam_url['PROD']
        iam_stub = get_grpc_channel(iam_token_service.IamTokenServiceStub,
                                    iam_endpoint,
                                    str(uuid.uuid4()))

        requesst = iam_token_service_pb2.CreateIamTokenRequest(
            jwt=encoded_token
        )
        resp = iam_stub.Create(requesst)
        self.expire = resp.expires_at.seconds
        self.token = resp.iam_token
        return resp.iam_token

    def get_token(self) -> str:
        now = int(time.time())
        if now > self.expire:
            iam_token = self.get_iam_token()
            return iam_token
        return self.token

    @property
    def iam_token(self):
        return self.token
