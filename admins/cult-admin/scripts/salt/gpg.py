#!/usr/bin/env python
''' Helpers for work with python-gnupg '''
''' author: aarseniev@yandex-team.ru '''
#TODO
# Script must work if -f -- not file, but string
# Add debug key and debug logging for it
import os
import sys
import gnupg
import argparse

home = os.path.expanduser("~") + '/.gnupg'
client = gnupg.GPG(gnupghome=home)

def get_data(data):
    ''' Get path to file
        Return text object '''
    with open(data,'r') as f:
        text = f.read()
    return text

def load_keys(path):
    ''' Get path to *.gpg files and gpg client
        Return list of fingerprints for loaded keys '''
    keys = []
    exported_keys = []
    for p in path:
        dir_list = os.listdir(p)
        for f in dir_list:
            if f.split('.')[-1] == 'gpg':
                keys.append(p + '/' + f)
    for key in keys:
        data = get_data(key)
        import_result = client.import_keys(data)
        exported_keys = exported_keys + import_result.fingerprints
    return exported_keys

def crypt(data, keys, action, trust=True):
    ''' Get text string, key fingerprints, gpg client, action (decrypt,encrypt), trust policy
        Return gpg object as result of gpg work '''
    if action == 'decrypt':
        result = client.decrypt(data, always_trust=trust)
    elif action == 'encrypt':
        result = client.encrypt(data, keys, always_trust=trust)
    return result

def main():
    parser = argparse.ArgumentParser(add_help=True, description="Utility for decrypt/encrypt files via gpg")
    parser.add_argument('-f', dest='file', action='store', required=True, help='Source file with data to be decrypted/encrypted')
    parser.add_argument('-a', dest='action', choices=['encrypt', 'decrypt'], required=True, help='what to do')
    parser.add_argument('--path', action='append', required=True, help='Path to dir with *.gpg public keys')
    args = parser.parse_args()

    fingers = load_keys(args.path)
    print crypt( get_data(args.file), load_keys(args.path), args.action)

if __name__ == '__main__':
    main()
