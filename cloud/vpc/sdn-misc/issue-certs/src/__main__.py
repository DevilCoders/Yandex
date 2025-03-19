#!/usr/bin/env python3

"""issue-certs.py issues certificates for oct-heads, puts them into
secret service and generates stub for bootstrap template.

Requirements:
 - Python modules from python.txt
 - keytool from JDK
 - Configured yc-secret-cli
"""

import argparse
import os
import logging
import sys
import tempfile

from typing import List, Optional

from yc_issue_cert.cluster import Cluster
from yc_issue_cert.config import load_config, Config
from yc_issue_cert.secrets import create_secret_factory, iter_hosts
from yc_issue_cert.secret_service import sync_secrets, dump_secrets_bootstrap, dump_secrets_salt
from yc_issue_cert.yc_crt import get_or_create_certs
from yc_issue_cert.utils import InternalError

import yc_issue_cert.secrets.oct           # noqa
import yc_issue_cert.secrets.monops        # noqa
import yc_issue_cert.secrets.vpc_node      # noqa
import yc_issue_cert.secrets.vpc_api       # noqa
import yc_issue_cert.secrets.ylb           # noqa
import yc_issue_cert.secrets.compute_node  # noqa


def parse_option_args(optval):
    return {k: v for k, v in (pair.split("=") for pair in optval.split(","))}


def parse_args():
    parser = argparse.ArgumentParser(description=__doc__)

    parser.add_argument("--debug", action="store_true")

    parser.add_argument("-c", "--config", dest="config", metavar="CONFIG",
                        help="Config file", default="config.yaml")

    parser.add_argument("--yc-crt-profile", metavar="PROFILE", default="prod",
                        help="Name of YC CRT profile defined in config.yaml")
    parser.add_argument("--reissue", action="store_true",
                        help="Force certificates recreation even if they already issued")
    parser.add_argument("--revoke", action="store_true",
                        help="Revoke old certificate")
    parser.add_argument("--revoke-expired", action="store_true",
                        help="Revoke only expired certificate")

    parser.add_argument("--skip-secrets", metavar="TYPE", nargs="+", default=[],
                        help="Skip secret types (they won't be touched in this run)")
    parser.add_argument("--only-secrets", metavar="TYPE", nargs="+",
                        help="Only update specified secret types")
    parser.add_argument("--only-hosts", metavar="HOST", nargs="+",
                        help="Only update specified hosts")
    parser.add_argument("--skip-hosts", metavar="HOST", nargs="+",
                        help="Skip specified hosts (they won't be touched in this run)")
    parser.add_argument("-o", "--option", dest="options", metavar="VAR=VAL[,VAR=VAL]",
                        type=parse_option_args, default={},
                        help="Set option for one of the secrets")
    parser.add_argument("cluster", metavar="CLUSTER",
                        help="Name of the cluster to issue certificates for")
    parser.add_argument("secret_group", metavar="GROUP",
                        help="Hostnames to issue certificates for")

    parser.add_argument("--save", metavar="DIRECTORY",
                        help="Save certificates instead of uploading to secret service")
    parser.add_argument("-f", "--format", metavar="FORMAT", choices=["bootstrap", "salt"],
                        default="salt",
                        help="Format for resulting secret config")

    return parser.parse_args()


def setup_logging(args):
    logging.basicConfig(format="%(message)s",
                        level=logging.INFO if not args.debug else logging.DEBUG)


def get_cluster(config: Config, cluster_name: str, only_hosts: Optional[List[str]], skip_hosts: Optional[List[str]]):
    try:
        cluster_config = config.clusters[cluster_name]
    except KeyError:
        raise RuntimeError("Cluster {!s} is not found in config".format(cluster_name))

    return Cluster(cluster_name, cluster_config, only_hosts, skip_hosts)


def get_secret_group(config: Config, sg_name: str):
    try:
        return config.secret_groups[sg_name]
    except KeyError:
        raise RuntimeError("Secrets group {!s} is not found in config".format(sg_name))


def main():
    args = parse_args()
    setup_logging(args)

    try:
        config = load_config(args.config)
        cluster = get_cluster(config, args.cluster, args.only_hosts, args.skip_hosts)
        secret_group = get_secret_group(config, args.secret_group)
        secret_factory = create_secret_factory(cluster, secret_group, args.options,
                                               args.skip_secrets, args.only_secrets)

        certificates = get_or_create_certs(config.yc_crt, iter_hosts(cluster, secret_group),
                                           args.yc_crt_profile, reissue=args.reissue, revoke=args.revoke,
                                           revoke_expired=args.revoke_expired,
                                           all_hosts_cert=secret_group.all_hosts_cert)

        if args.save:
            os.makedirs(args.save, exist_ok=True)
            secret_factory.create(args.save, certificates)
            return

        with tempfile.TemporaryDirectory(prefix="yc-issue-certs") as tmp_dir:
            secrets_data = secret_factory.create(tmp_dir, certificates)
            secrets = sync_secrets(config.secret_service, secrets_data, cluster,
                                   args.secret_group)

        if args.format == "bootstrap":
            dump_secrets_bootstrap(args.secret_group, secrets, sys.stdout)
        elif args.format == "salt":
            dump_secrets_salt(args.secret_group, secrets, sys.stdout)
    except (InternalError, NotImplementedError) as e:
        logging.exception("%s", e)
    except RuntimeError as e:
        logging.error("%s", e)


if __name__ == "__main__":
    main()
