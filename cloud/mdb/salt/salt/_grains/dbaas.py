#!/usr/bin/env python
import json


def dbaas():
    try:
        with open('/etc/dbaas-container.json') as inp:
            return {'dbaas': json.loads(inp.read())}
    except Exception:
        return {'dbaas': {}}


if __name__ == '__main__':
    from pprint import pprint
    pprint(ya())
