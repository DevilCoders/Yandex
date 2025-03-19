#!/usr/bin/env python3

import sys
import jwt
import time

if len(sys.argv) != 3:
	print(f"Usage: python3 {sys.argv[0]} <service-account-id> <key-id>", file=sys.stderr)
	sys.exit(1)

service_account_id, key_id = sys.argv[1:]
print("Input private key (press Ctrl+D when you're done):", file=sys.stderr)

private_key = sys.stdin.read()

# See https://cloud.yandex.ru/docs/iam/operations/iam-token/create-for-sa#via-jwt
now = int(time.time())
payload = {
    "aud": "https://iam.api.cloud.yandex.net/iam/v1/tokens",
    "iss": service_account_id,
    "iat": now,
    "exp": now + 360
}

encoded_token = jwt.encode(
    payload,
    private_key,
    algorithm="PS256",
    headers={"kid": key_id}
)

print(encoded_token)
