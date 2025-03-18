#!/skynet/python/bin/python
import os
import sys

sys.path.append(os.path.abspath(os.path.join(os.path.dirname(__file__), '..', '..')))

import gencfg

from core.db import CURDB
from core.card.node import CardNode, load_card_node


ALLOWED_COLLISIONS = set(["unknown",])


def main():
    datafile = os.path.join(CURDB.HDATA_DIR, 'models.yaml')
    schemefile = os.path.join(CURDB.SCHEMES_DIR, 'cpumodels.yaml')

    cpu_models = load_card_node(datafile, schemefile)

    botmodels = set()
    collisions = set()
    for cpu_model in cpu_models:
        for botmodel in cpu_model['botmodel']:
            if botmodel in botmodels:
                collisions.add(botmodel)
            else:
                botmodels.add(botmodel)

    bad_collisions = collisions.difference(ALLOWED_COLLISIONS)
    if len(bad_collisions) > 0:
        print("Collisions in botmodels")
        print(list(bad_collisions))
        return -1

    return 0

if __name__ == '__main__':
    ret_value = main()
    sys.exit(ret_value)
