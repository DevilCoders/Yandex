#!/usr/bin/python

import os

def _roles():
    grains = {}
    if os.path.exists("/etc/yc-roles"):
        grains = { 'role': [] }
        with open("/etc/yc-roles", "r") as role_file:
            for role in role_file:
                role = role.rstrip()
                grains['role'].append(role)
    return grains

def main():
    grains = _roles()
    return grains

if __name__ == '__main__':
    main()
