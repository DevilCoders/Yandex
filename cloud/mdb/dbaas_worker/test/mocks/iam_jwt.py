"""
Simple iam_jwt mock
"""

import time

from yandex.cloud.priv.iam.v1 import iam_token_service_pb2


def create_token():
    """
    CreateIamToken mock
    """
    res = iam_token_service_pb2.CreateIamTokenResponse()
    res.iam_token = 'dummy-token'
    res.expires_at.FromSeconds(int(time.time()) + 24 * 3600)
    return res


def iam_jwt(mocker, _):
    """
    Setup iam_jwt mock
    """
    stub = mocker.patch(
        'cloud.mdb.internal.python.compute.iam.jwt.iam_jwt.iam_token_service_pb2_grpc.IamTokenServiceStub'
    )
    jwt = mocker.patch('cloud.mdb.internal.python.compute.iam.jwt.iam_jwt.jwt')
    jwt.encode.side_effect = lambda payload, *_args, **_kwargs: payload['iss']
    stub.return_value.Create.side_effect = lambda *_args, **_kwargs: create_token()
    stub.return_value.CreateForServiceAccount.side_effect = lambda *_args, **_kwargs: create_token()
