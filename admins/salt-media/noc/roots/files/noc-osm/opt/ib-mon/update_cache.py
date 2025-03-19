#!/usr/bin/env python3
import logging
import os.path
import os
import argparse

import lib


if __name__ == '__main__':
    parser = argparse.ArgumentParser()
    parser.add_argument("--config", help="config file", action="store")
    parser.add_argument("--debug", help="enable debug", action="store_true")
    parser.add_argument("--fqdn", help="use this FQDN instead local host name", action="store")
    options = parser.parse_args()
    level = logging.WARNING
    if options.debug:
        level = logging.DEBUG
    logging.basicConfig(level=level,
                        format="%(asctime)s - %(filename)s:%(lineno)d - %(funcName)s() - %(levelname)s - %(message)s")

    vardir = os.path.join(lib.basedir(), "var")
    fqdn = options.fqdn if options.fqdn else lib.my_fqdn()
    config = lib.read_config(options.config)
    token = config['auth']['invapi_token']
    lib.get_ufm_hosts_for_cluster_with_host_cached(fqdn, token, vardir, cachettl=100)

