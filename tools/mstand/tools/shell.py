#!/usr/bin/env python2
import sys

# noinspection PyPackageRequirements
import IPython

# noinspection PyUnresolvedReferences
import experiment_pool as epool
import experiment_pool.pool_helpers as pool_helpers

pool = pool_helpers.load_pool(sys.argv[1])


def save():
    pool_helpers.dump_pool(pool, sys.argv[1])


def wq():
    save()
    sys.exit(0)


IPython.embed()
