import time

from cloud.gauthling.auth_token_protobuf.token_pb2 import SignedToken, Token


def is_expired(signed_token_bytes):
    signed_token = SignedToken()
    signed_token.ParseFromString(signed_token_bytes)
    token = Token()
    token.ParseFromString(signed_token.token)
    # To avoid token expiry during a request reduce actual expiry time by 5 minutes
    expiry_time = token.expires_at - 300
    return int(time.time()) > expiry_time
