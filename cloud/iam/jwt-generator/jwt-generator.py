# -*- coding: utf-8 -*-

import argparse
import time
import jwt


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('-k', '--key-id', help='Key identitifier', required=True)
    parser.add_argument('-s', '--service-account-id', help='Service account identifier', required=True)
    parser.add_argument('-p', '--private-key-file', help='Path to file with private key', required=True)

    args = parser.parse_args()

    with open(args.private_key_file, 'r') as private:
        private_key = private.read()

    now = int(time.time())
    payload = {
        'aud': 'https://iam.api.cloud.yandex.net/iam/v1/tokens',
        'iss': args.service_account_id,
        'iat': now,
        'exp': now + 360}

    encoded_token = jwt.encode(
        payload,
        private_key,
        algorithm='PS256',
        headers={'kid': args.key_id})

    print(encoded_token.decode('utf-8'))


if __name__ == "__main__":
    main()
