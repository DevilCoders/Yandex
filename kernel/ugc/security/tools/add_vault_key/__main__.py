#!/usr/bin/env python
# -*- coding: utf-8 -*-
import argparse
import getpass

from kernel.ugc.security.tools.add_vault_key import oauth
from kernel.ugc.security.tools.add_vault_key import vault
from kernel.ugc.security.tools.add_vault_key import generate

DEFAULT_DELAY_DAYS = 14
DEFAULT_KEYCHAIN = 'ugc-keys'


def main():
    parser = argparse.ArgumentParser(description='Generate new encryption key')
    parser.add_argument('-d', '--delay', help='Days before key\'s start date',
                        default=DEFAULT_DELAY_DAYS)
    parser.add_argument('-k', '--keychain', help='Keychain id',
                        default=DEFAULT_KEYCHAIN)
    args = parser.parse_args()

    user = getpass.getuser()
    token = oauth.get_token(user)
    key_name = generate.get_variable_name(args.delay)
    key = generate.get_key()
    secret_data = {key_name: key}

    client = vault.VaultClient(token)
    client.create_secret(args.keychain, key_name, 'ugc-key', secret_data)


if __name__ == "__main__":
    main()
